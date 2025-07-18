#include <pch.h>

#include <meshoptimizer.h>

#include "Model.h"
#include "Log.h"
#include "Texture.h"
#include "renderer.h"
#include "Vertex.h"
#include "vk_utils.h"

Cravillac::Model::Model()
{
}
Cravillac::Model::~Model()
{
}

void Cravillac::Model::LoadModel(const std::shared_ptr<Renderer>& renderer, const std::string& path)
{
    this->m_renderer = renderer;

    m_resourceManager = new ResourceManager(m_renderer);
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

    if (result != cgltf_result_success)
        printl(Log::LogLevel::Error,"[CGLTF] Failed to parse gltf file");
    else
        printl(Log::LogLevel::Info,"[CGLTF] Successfully parsed gltf file");

    result = cgltf_load_buffers(&options, data, path.c_str());

    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        printl(Log::LogLevel::Error,"[CGLTF] Failed to load buffers");
    }
    else
    {
        printl(Log::LogLevel::Info,"[CGLTF] Successfully loaded buffers");
    }

    cgltf_scene *scene = data->scene;

    if (!scene)
    {
        printl(Log::LogLevel::Error,"[CGLTF] No scene found in gltf file");
    }
    else
    {
        printl(Log::LogLevel::Info,"[CGLTF] Scene found in gltf file");
        m_dirPath = path.substr(0, path.find_last_of("/"));

        for (size_t i = 0; i < (scene->nodes_count); i++)
        {
            Transformation transform;
            ProcessNode(scene->nodes[i], data, m_vertices, m_indices, transform);
        }
        // no of nodes
        printl(Log::LogLevel::InfoDebug,"[CGLTF] No of nodes in the scene: {} ", scene->nodes_count);

        SetBuffers();

        printl(Log::LogLevel::Info,"[CGLTF] Successfully loaded gltf file");
    }

    ValidateResources();

    cgltf_free(data);
}

DirectX::XMFLOAT3X3 Cravillac::Model::ComputeNormalMatrix(const DirectX::XMMATRIX& worldMatrix)
{
    DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, worldMatrix));
    DirectX::XMFLOAT3X3 result;
    DirectX::XMStoreFloat3x3(&result, normalMatrix);
    return result;
}

void Cravillac::Model::ProcessNode(cgltf_node *node, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform)
{
    Transformation localTransform;
    localTransform.Matrix = parentTransform.Matrix;
    if (node->has_scale) 
    {
        localTransform.Matrix *= DirectX::XMMatrixScaling(node->scale[0], node->scale[1], node->scale[2]);
    }
    if (node->has_rotation) 
    {
        DirectX::XMVECTOR quat = DirectX::XMVectorSet(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
        localTransform.Matrix *= DirectX::XMMatrixRotationQuaternion(quat);
    }
    if (node->has_translation) 
    {
        localTransform.Matrix *= DirectX::XMMatrixTranslation(node->translation[0], node->translation[1], node->translation[2]);
    }

    //if (node->camera)
    //{
    //    const cgltf_camera_perspective& perspective = node->camera->data.perspective;

    //    float yfov = static_cast<float>(perspective.yfov);
    //    float aspectRatio = static_cast<float>(perspective.aspect_ratio);
    //    float znear = static_cast<float>(perspective.znear);
    //    float zfar = (perspective.has_zfar) ? static_cast<float>(perspective.zfar) : FLT_MAX;

    //    camera->InitAsPerspective(yfov, m_renderer->m_width, m_renderer->m_width/aspectRatio, znear, zfar);    
    //}

    //localTransform.Matrix = scaleMatrix * rotationMatrix * translationMatrix;
    //Log::InfoDebug("[CGLTF] parentTransform Matrix: {}", localTransform.Matrix);

    // Process meshInfo if exists
    if (node->mesh)
    {
        for (size_t i = 0; i < (node->mesh->primitives_count); i++)
        {
            //Log::InfoDebug("[CGLTF] parentTransform Matrix: {}", localTransform.Matrix);
            /*Log::InfoDebug("[CGLTF] parentTransform Position: {}", localTransform.Position);
            Log::InfoDebug("[CGLTF] parentTransform Rotation: {}", localTransform.Rotation);
            Log::InfoDebug("[CGLTF] parentTransform Scale: {}", localTransform.Scale);*/

            ProcessMesh(&node->mesh->primitives[i], vertices, indices, localTransform);   // remember that the indices and the vertices passed here are m_indices and m_vertices refs
        }
    }

    // Recursively process child nodes
    for (size_t i = 0; i < node->children_count; i++)
    {
        ProcessNode(node->children[i], data, vertices, indices, localTransform);
    }
}

void Cravillac::Model::ProcessMesh(cgltf_primitive *primitive, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform)
{
    u32 vertexOffset = static_cast<u32>(vertices.size());
    u32 indexOffset = static_cast<u32>(indices.size());

    std::vector<Vertex> tempVertices;
    std::vector<u32> tempIndices;

    if (primitive->type != cgltf_primitive_type_triangles)
    {
        printl(Log::LogLevel::Warn,"[CGLTF] Primitive type is not triangles");
        return;
    }

    if (primitive->indices == nullptr)
    {
        printl(Log::LogLevel::Error,"[CGLTF] Primitive has no indices");
        return;
    }

    if (primitive->material == nullptr)
    {
        printl(Log::LogLevel::Error,"[CGLTF] Primitive has no material");
        return;
    }

    MeshInfo meshInfo;

    meshInfo.transform = parentTransform;
    meshInfo.normalMatrix = ComputeNormalMatrix(parentTransform.Matrix);

    // Get attributes
    cgltf_attribute *pos_attribute = nullptr;
    cgltf_attribute *tex_attribute = nullptr;
    cgltf_attribute *norm_attribute = nullptr;

    for (int i = 0; i < primitive->attributes_count; i++)
    {
        if (strcmp(primitive->attributes[i].name, "POSITION") == 0)
        {
            pos_attribute = &primitive->attributes[i];
        }   
        if (strcmp(primitive->attributes[i].name, "TEXCOORD_0") == 0)
        {
            tex_attribute = &primitive->attributes[i];
        }
        if (strcmp(primitive->attributes[i].name, "NORMAL") == 0)
        {
            norm_attribute = &primitive->attributes[i];
        }
    }

    if (!pos_attribute || !tex_attribute || !norm_attribute)
    {
        printl(Log::LogLevel::Warn,"[CGLTF] Missing attributes in primitive");
        return;
    }

    size_t vertexCount;
    vertexCount = pos_attribute->data->count;
    size_t indexCount;
    indexCount = primitive->indices->count;

    for (size_t i = 0; i < vertexCount; i++)
    {
        Vertex vertex = {};

        // Read original vertex data
        if (cgltf_accessor_read_float(pos_attribute->data, i, &vertex.pos.x, 3) == 0)
        {
            printl(Log::LogLevel::Warn,"[CGLTF] Unable to read Position attributes!");
        }
        if (cgltf_accessor_read_float(tex_attribute->data, i, &vertex.texCoord.x, 2) == 0)
        {
            printl(Log::LogLevel::Warn,"[CGLTF] Unable to read Texture attributes!");
        }
        if (cgltf_accessor_read_float(norm_attribute->data, i, &vertex.normal.x, 3) == 0)
        {
            printl(Log::LogLevel::Warn,"[CGLTF] Unable to read Normal attributes!");
        }
        tempVertices.push_back(vertex);
        //vertices.push_back(vertex);
    }

    for (size_t i = 0; i < indexCount; i++)
    {
        tempIndices.push_back(cgltf_accessor_read_index(primitive->indices, i));
        //indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
    }

    // material

    cgltf_material *material = primitive->material; //remember this is pointing towards the material pointer. In gltf as long as the material remains the same, the pointer is the same even for diff primitives

    if (!materialLookup.contains(material)) // if the hash table doesn't have the material hash
    {
        Material mat = {};

        HRESULT hr = E_FAIL;

        // map texture types to their respective textures
        std::unordered_map<TextureType, cgltf_texture_view*> textureMap;

        //the following code for materials is very much unoptimised. It should only look for materials once, make a texture, sampler and save it in a map, not per primitive
        //#TODO

        if (material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
            // Map base color texture (albedo)
            if (pbr->base_color_texture.texture)
            {
                textureMap[TextureType::ALBEDO] = &pbr->base_color_texture;
            }
            if (pbr->metallic_roughness_texture.texture)
            {
                // Map metallic-roughness texture
                textureMap[TextureType::METALLIC_ROUGHNESS] = &pbr->metallic_roughness_texture;
                //mat.MaterialName = 
            }
        }

        if (material->normal_texture.texture)
        {
            // Map normal texture
            textureMap[TextureType::NORMAL] = &material->normal_texture;
        }
        if (material->emissive_texture.texture)
        {
            // Map emissive texture
            textureMap[TextureType::EMISSIVE] = &material->emissive_texture;
        }

        if (material->occlusion_texture.texture)
        {
            // Map occlusion texture
            textureMap[TextureType::AO] = &material->occlusion_texture;
        }
        if (material->has_pbr_specular_glossiness)
        {
            cgltf_pbr_specular_glossiness* pbr = &material->pbr_specular_glossiness;
            if (pbr->specular_glossiness_texture.texture)
            {
                //textureMap[TextureType::SPECULAR_GLOSSINESS] = &material->tex
            }
        }

        // Load all textures from the map if they haven't been loaded before
        for (const auto& [type, view] : textureMap)
        {
            static int i = 0;
            std::string textureIdentifier = std::to_string(static_cast<int>(type)); // Unique identifier for texture type
            std::string imageName = view->texture->image->uri;

            if (!loadedTextures.contains(imageName)) // If not already loaded
            {
                hr = LoadMaterialTexture(mat, view, type);
                if (FAILED(hr))
                {
                    //printl(Log::LogLevel::Error,"[Texture] Failed to load texture of type {} with name {} # {} ", textureIdentifier, imageName, i++);
                }
                else
                {
                    //printl(Log::LogLevel::Info,"[Texture] Loaded texture of type {} with name {} # {}", textureIdentifier, imageName, i++);
                    loadedTextures.insert(imageName); // Mark this texture type as loaded
                }
            }
            else
            {
                //printl(Log::LogLevel::Warn, "[Texture] Skipping already loaded texture of type {} with name {} ", textureIdentifier, imageName);
            }
        }
        m_materials.push_back(mat);
        materialLookup[material] = m_materials.size() - 1;
        meshInfo.materialIndex = static_cast<u32>(materialLookup[material]);
    }

    meshInfo.materialIndex = static_cast<u32>(materialLookup[material]);
    meshInfo.transform = parentTransform;
    meshInfo.startIndex = indexOffset;
    meshInfo.startVertex = vertexOffset;
    meshInfo.vertexCount = vertexCount;
    meshInfo.indexCount = indexCount;

    Mesh mesh;
    mesh.vertices = tempVertices;
    mesh.indices = tempIndices;
    mesh.vertexCount = static_cast<u32>(vertexCount);
    mesh.indexCount = static_cast<u32>(indexCount);

    OptimiseMesh(meshInfo, mesh);
    ProcessMeshlets(mesh);
    m_meshes.push_back(meshInfo);
}

void Cravillac::Model::OptimiseMesh(MeshInfo& meshInfo, Mesh& mesh)
{
    size_t indexCount = meshInfo.indexCount;
    size_t vertexCount = meshInfo.vertexCount;

    std::vector<unsigned int> remap(indexCount);
    size_t optVertexCount = meshopt_generateVertexRemap(remap.data(), mesh.indices.data(), indexCount, mesh.vertices.data(), vertexCount, sizeof(Vertex));

    std::vector<u32> optIndices;
    std::vector<Vertex> optVertices;
    optIndices.resize(indexCount);
    optVertices.resize(optVertexCount);

    // Optimisation 1 - Remove duplicate vertices
    meshopt_remapIndexBuffer(optIndices.data(), mesh.indices.data(), indexCount, remap.data());
    meshopt_remapVertexBuffer(optVertices.data(), mesh.vertices.data(), vertexCount, sizeof(Vertex), remap.data());

    // Optimisation 2 - improve the locality of the vertices
    meshopt_optimizeVertexCache(optIndices.data(), optIndices.data(), indexCount, optVertexCount);

    // Optimization 3 - reduce pixel overdraw
    meshopt_optimizeOverdraw(optIndices.data(), optIndices.data(), indexCount, &(optVertices[0].pos.x), optVertexCount, sizeof(Vertex), static_cast<float>(1.05));

    // Optimization 4 - optimize access to the vertex buffer
    meshopt_optimizeVertexFetch(optVertices.data(), optIndices.data(), indexCount, optVertices.data(), optVertexCount, sizeof(Vertex));

    // Optimization 5 - create simplified version of the model
    float threshold = 0.5f;
    size_t targetIndexCount = (size_t)(threshold * indexCount);
    float targetError = 0.3f;

    std::vector<u32> simplifiedIndices(optIndices.size());
    size_t optIndexCount = meshopt_simplify(simplifiedIndices.data(), optIndices.data(), indexCount,
                                            &(optVertices[0].pos.x), optVertexCount, sizeof(Vertex), targetIndexCount,
                                            targetError);
    simplifiedIndices.resize(optIndexCount);

    m_indices.insert(m_indices.end(), simplifiedIndices.begin(), simplifiedIndices.end());
    m_vertices.insert(m_vertices.end(), optVertices.begin(), optVertices.end());

    meshInfo.indexCount = optIndexCount;
    meshInfo.vertexCount = optVertexCount;

    mesh.vertices = optVertices;
    mesh.indices = optIndices;
    mesh.vertexCount = static_cast<u32>(optVertexCount);
    mesh.indexCount = static_cast<u32>(optIndexCount);
}

void Cravillac::Model::ProcessMeshlets(Mesh& mesh)
{
    Meshlet meshlet = {};
    std::vector<u8> meshletVertices(mesh.vertexCount, 0xff);

    for (size_t i = 0; i <mesh.indexCount; i+=3)
    {
        unsigned int a = mesh.indices[i + 0];
        unsigned int b = mesh.indices[i + 1];
        unsigned int c = mesh.indices[i + 2];

        u8& av = meshletVertices[a];
        u8& bv = meshletVertices[b];
        u8& cv = meshletVertices[c];

        if ((meshlet.vertexCount + (av==0xff) + (bv==0xff) + (cv==0xff) > 64) || (meshlet.triangleCount + 1 > 126/3))
        {
            m_meshlets.push_back(meshlet);
            meshlet = {};
            memset(meshletVertices.data(), 0xff, meshletVertices.size());
        }

        if (av==0xff)
        {
            av = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = a;
        }
        if (bv==0xff)
        {
            bv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = b;
        }
        if (cv==0xff)
        {
            cv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = c;
        }
        meshlet.indices[meshlet.triangleCount * 3 + 0] = av;
        meshlet.indices[meshlet.triangleCount * 3 + 1] = bv;
        meshlet.indices[meshlet.triangleCount * 3 + 2] = cv;
        meshlet.triangleCount++;
    }
    if (meshlet.triangleCount)
    {
        m_meshlets.push_back(meshlet);
    }
}

bool Cravillac::Model::LoadMaterialTexture(Material &mat, const cgltf_texture_view *textureView, const TextureType type)
{
    if (textureView && textureView->texture && textureView->texture->image)
    {
        cgltf_image *image = textureView->texture->image;
        std::string path = m_dirPath + "/" + std::string(image->uri);

        Texture tex;
        tex.LoadTexture(m_renderer, path.c_str());
        switch (type)
        {
        case TextureType::ALBEDO:
            mat.AlbedoView = tex.m_texImageView;
            mat.HasAlbedo = true;
            mat.AlbedoPath = path;
            modelTextures.push_back(tex);
            return true;
        // case TextureType::NORMAL:
        //     mat.NormalView = tex.m_texImageView;
        //     mat.HasNormal = true;
        //     mat.NormalPath = path;
        //     modelTextures.push_back(tex);
        //     return S_OK;
        // case TextureType::METALLIC_ROUGHNESS:
        //     mat.MetallicRoughnessView = tex.m_texImageView;
        //     mat.HasMetallicRoughness = true;
        //     mat.MetallicRoughnessPath = path;
        //     return S_OK;
        // case TextureType::EMISSIVE:
        //     mat.EmissiveView = tex.m_texImageView;
        //     mat.HasEmissive = true;
        //     mat.EmissivePath = path;
        //     modelTextures.push_back(tex);
        //     return S_OK;
        // case TextureType::AO:
        //     mat.AOView = tex.m_texImageView;
        //     mat.HasAO = true;
        //     mat.AOPath = path;
        //     modelTextures.push_back(tex);
        //     return S_OK;
        default:
            //printl(Log::LogLevel::Warn,"[Texture] Unknown texture type.");
            return false;
        }
    }
    // Handle missing texture or image
    printl(Log::LogLevel::Warn,"[Texture] Texture or image not found for material : {}", std::to_string(static_cast<int>(type)));
    return false;
}

void Cravillac::Model::SetBuffers()
{
    vk::DeviceSize bufferSize;
    vk::Buffer stagingBuffer{};
    vk::DeviceMemory stagingBufferMemory{};
    void* data;
    // meshlet buffer stuff
#if MESH_SHADING
    bufferSize = m_meshlets.size() * sizeof(Meshlet);

    stagingBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        .build(stagingBufferMemory);

    data = nullptr;
    vkMapMemory(m_renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_meshlets.data(), bufferSize);
    vkUnmapMemory(m_renderer->m_device, stagingBufferMemory);

    m_meshletBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build(m_meshletMemory);

    CopyBuffer(m_renderer->m_device, m_renderer->m_commandPool, m_renderer->m_graphicsQueue, stagingBuffer, m_meshletBuffer,
               bufferSize);
    vkDestroyBuffer(m_renderer->m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_renderer->m_device, stagingBufferMemory, nullptr);
#else
    // vertex buffer
    bufferSize = sizeof(Vertex) * m_vertices.size();

    stagingBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc| vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        .build(stagingBufferMemory);

    vkMapMemory(m_renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_renderer->m_device, stagingBufferMemory);

    m_vertexBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build(m_vertexMemory);

    CopyBuffer(m_renderer->m_device, m_renderer->m_commandPool, m_renderer->m_graphicsQueue, stagingBuffer, m_vertexBuffer,
               bufferSize);
#endif

    // index buffer
    bufferSize = m_indices.size() * sizeof(m_indices[0]);

    stagingBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        .build(stagingBufferMemory);

    data = nullptr;
    vkMapMemory(m_renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_renderer->m_device, stagingBufferMemory);

    m_indexBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build(m_indexMemory);

    CopyBuffer(m_renderer->m_device, m_renderer->m_commandPool, m_renderer->m_graphicsQueue, stagingBuffer, m_indexBuffer,
               bufferSize);
    vkDestroyBuffer(m_renderer->m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_renderer->m_device, stagingBufferMemory, nullptr);

    // unlike DX11, samplers handled independent of pipeline, so they are handled by the texture class.
    // will be handled during the desc layout stuff. Creating a large texture array,
    // making it index with the corresponding material index and using it directly in shader i.e descriptor indexing
}

void Cravillac::Model::ValidateResources() const
{
    printl(Log::LogLevel::Info,
                "[CGLTF] Validating Model Resources: \nVertices: {} \nIndices: {}\nMaterials: {}\nMeshes: {}",
                m_vertices.size(), m_indices.size(), m_materials.size(), m_meshes.size());
    // Check camera position
    //DirectX::XMFLOAT3 pos;
    /*XMStoreFloat3(&pos, camera->GetPosition());
    Log::Info("[D3D] Camera Position: " + std::to_string(pos.x) + ", " +
              std::to_string(pos.y) + ", " + std::to_string(pos.z));*/
}