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

void Cravillac::Model::LoadModel(const std::shared_ptr<Renderer>& renderer, std::string path)
{
    this->renderer = renderer;

    m_resourceManager = new ResourceManager(renderer);
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

    if (result != cgltf_result_success)
        Log::Error("[CGLTF] Failed to parse gltf file");
    else
        Log::Info("[CGLTF] Successfully parsed gltf file");

    result = cgltf_load_buffers(&options, data, path.c_str());

    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        Log::Error("[CGLTF] Failed to load buffers");
    }
    else
    {
        Log::Info("[CGLTF] Successfully loaded buffers");
    }

    cgltf_scene *scene = data->scene;

    if (!scene)
    {
        Log::Error("[CGLTF] No scene found in gltf file");
    }
    else
    {
        Log::Info("[CGLTF] Scene found in gltf file");
        m_dirPath = path.substr(0, path.find_last_of("/"));

        for (size_t i = 0; i < (scene->nodes_count); i++)
        {
            Transformation transform;
            ProcessNode(scene->nodes[i], data, m_vertices, m_indices, transform);
        }
        // no of nodes
        Log::InfoDebug("[CGLTF] No of nodes in the scene: ", scene->nodes_count);

        SetBuffers();

        Log::Info("[CGLTF] Successfully loaded gltf file");
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

    //    camera->InitAsPerspective(yfov, renderer->m_width, renderer->m_width/aspectRatio, znear, zfar);    
    //}

    //localTransform.Matrix = scaleMatrix * rotationMatrix * translationMatrix;
    //Log::InfoDebug("[CGLTF] parentTransform Matrix: {}", localTransform.Matrix);

    // Process mesh if exists
    if (node->mesh)
    {
        for (size_t i = 0; i < (node->mesh->primitives_count); i++)
        {
            //Log::InfoDebug("[CGLTF] parentTransform Matrix: {}", localTransform.Matrix);
            /*Log::InfoDebug("[CGLTF] parentTransform Position: {}", localTransform.Position);
            Log::InfoDebug("[CGLTF] parentTransform Rotation: {}", localTransform.Rotation);
            Log::InfoDebug("[CGLTF] parentTransform Scale: {}", localTransform.Scale);*/

            ProcessMesh(&node->mesh->primitives[i], data, vertices, indices, localTransform);   // remember that the indices and the vertices passed here are m_indices and m_vertices refs
        }
    }

    // Recursively process child nodes
    for (size_t i = 0; i < node->children_count; i++)
    {
        ProcessNode(node->children[i], data, vertices, indices, localTransform);
    }
}

void Cravillac::Model::ProcessMesh(cgltf_primitive *primitive, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform)
{
    u32 vertexOffset = vertices.size();
    u32 indexOffset = indices.size();

    std::vector<Vertex> tempVertices;
    std::vector<u32> tempIndices;

    if (primitive->type != cgltf_primitive_type_triangles)
    {
        Log::Warn("[CGLTF] Primitive type is not triangles");
        return;
    }

    if (primitive->indices == nullptr)
    {
        Log::Error("[CGLTF] Primitive has no indices");
        return;
    }

    if (primitive->material == nullptr)
    {
        Log::Error("[CGLTF] Primitive has no material");
        return;
    }

    Primitive prim;

    prim.transform = parentTransform;
    prim.normalMatrix = ComputeNormalMatrix(parentTransform.Matrix);

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
        Log::Warn("[CGLTF] Missing attributes in primitive");
        return;
    }

    int vertexCount;
    vertexCount = pos_attribute->data->count;
    int indexCount;
    indexCount = primitive->indices->count;

    for (int i = 0; i < vertexCount; i++)
    {
        Vertex vertex = {};

        // Read original vertex data
        if (cgltf_accessor_read_float(pos_attribute->data, i, &vertex.pos.x, 3) == 0)
        {
            Log::Warn("[CGLTF] Unable to read Position attributes!");
        }
        if (cgltf_accessor_read_float(tex_attribute->data, i, &vertex.texCoord.x, 2) == 0)
        {
            Log::Warn("[CGLTF] Unable to read Texture attributes!");
        }
        if (cgltf_accessor_read_float(norm_attribute->data, i, &vertex.normal.x, 3) == 0)
        {
            Log::Warn("[CGLTF] Unable to read Normal attributes!");
        }
        //tempVertices.push_back(vertex);
        vertices.push_back(vertex);
    }

    for (int i = 0; i < indexCount; i++)
    {
        //tempIndices.push_back(cgltf_accessor_read_index(primitive->indices, i));
        indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
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
                    Log::Error("[Texture] Failed to load texture of type " + textureIdentifier + " with name: " + imageName);
                    Log::InfoDebug("  #", i++);
                }
                else
                {
                    Log::Info("[Texture] Loaded texture of type " + textureIdentifier + " with name: " + imageName);
                    Log::InfoDebug("  #", i++);
                    loadedTextures.insert(imageName); // Mark this texture type as loaded
                }
            }
            else
            {
                Log::Info("[Texture] Skipping already loaded texture of type "+ textureIdentifier + " with name: " + imageName);
            }
        }
        m_materials.push_back(mat);
        materialLookup[material] = m_materials.size() - 1;
        prim.materialIndex = materialLookup[material];
    }

    prim.materialIndex = materialLookup[material];
    prim.transform = parentTransform;
    prim.startIndex = indexOffset;
    prim.startVertex = vertexOffset;
    prim.vertexCount = vertexCount;
    prim.indexCount = indexCount;

    //OptimiseMesh(prim, tempVertices, tempIndices);
    m_primitives.push_back(prim);
}

void Cravillac::Model::OptimiseMesh(Primitive& prim, std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
    size_t indexCount = prim.indexCount;
    size_t vertexCount = prim.vertexCount;

    std::vector<unsigned int> remap(indexCount);
    size_t optVertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(), indexCount, vertices.data(), vertexCount, sizeof(Vertex));

    std::vector<u32> optIndices;
    std::vector<Vertex> optVertices;
    optIndices.resize(indexCount);
    optVertices.resize(vertexCount);

    // Optimisation 1 - Remove duplicate vertices
    meshopt_remapIndexBuffer(optIndices.data(), indices.data(), indexCount, remap.data());
    meshopt_remapVertexBuffer(optVertices.data(), vertices.data(), vertexCount, sizeof(Vertex), remap.data());

    // Optimisation 2 - improve the locality of the vertices
    meshopt_optimizeVertexCache(optIndices.data(), optIndices.data(), indexCount, optVertexCount);

    // Optimization 3 - reduce pixel overdraw
    meshopt_optimizeOverdraw(optIndices.data(), optIndices.data(), indexCount, &(optVertices[0].pos.x), optVertexCount, sizeof(Vertex), 1.05);

    // Optimization 4 - optimize access to the vertex buffer
    meshopt_optimizeVertexFetch(optVertices.data(), optIndices.data(), indexCount, optVertices.data(), optVertexCount, sizeof(Vertex));

    // Optimization 5 - create simplified version of the model
    float threshold = 0.5f;
    size_t targetIndexCount = (size_t)(indexCount * threshold);
    float targetError = 0.3f;

    std::vector<u32> simplifiedIndices(optIndices.size());
    size_t optIndexCount = meshopt_simplify(simplifiedIndices.data(), optIndices.data(), indexCount,
                                            &(optVertices[0].pos.x), optVertexCount, sizeof(Vertex), targetIndexCount,
                                            targetError);
    m_indices.insert(m_indices.end(), simplifiedIndices.begin(), simplifiedIndices.end());
    m_vertices.insert(m_vertices.end(), optVertices.begin(), optVertices.end());

    prim.indexCount = optIndexCount;
    prim.vertexCount = optVertexCount;
}

HRESULT Cravillac::Model::LoadMaterialTexture(Material &mat, const cgltf_texture_view *textureView, const TextureType type)
{
    if (textureView && textureView->texture && textureView->texture->image)
    {
        cgltf_image *image = textureView->texture->image;
        std::string path = m_dirPath + "/" + std::string(image->uri);

        Texture tex;
        tex.LoadTexture(renderer, path.c_str());
        switch (type)
        {
        case TextureType::ALBEDO:
            mat.AlbedoView = tex.m_texImageView;
            mat.HasAlbedo = true;
            mat.AlbedoPath = path;
            modelTextures.push_back(tex);
            return S_OK;
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
            Log::Warn("[Texture] Unknown texture type.");
            return E_FAIL;
        }
    }
    // Handle missing texture or image
    Log::Warn("[Texture] Texture or image not found for this material.");
    return E_FAIL;
}

void Cravillac::Model::SetBuffers()
{
    // vertex buffer
    VkDeviceSize bufferSize = sizeof(Vertex) * m_vertices.size();
    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    stagingBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        .build(stagingBufferMemory);

    void* data{};
    vkMapMemory(this->renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(renderer->m_device, stagingBufferMemory);

    m_vertexBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        .build(m_vertexMemory);

    CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);

    // index buffer
    bufferSize = m_indices.size() * sizeof(m_indices[0]);

    stagingBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        .setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        .build(stagingBufferMemory);

    data = nullptr;
    vkMapMemory(renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(renderer->m_device, stagingBufferMemory);

    m_indexBuffer = m_resourceManager->CreateBufferBuilder()
        .setSize(bufferSize)
        .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        .setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        .build(m_indexMemory);

    CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);
    vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
    vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

    // unlike DX11, samplers handled independent of pipeline, so they are handled by the texture class.
    // will be handled during the desc layout stuff. Creating a large texture array,
    // making it index with the corresponding material index and then using it directly in shader i.e using descriptor indexing


    // Clear CPU data
    /*vertices.clear();
    indices.clear();*/
}

bool Cravillac::Model::SetTexResources(uint32_t materialIndex)
{
    //renderer->m_context->PSSetConstantBuffers(0, 1, m_materialBuffer.GetAddressOf());
    //renderer->m_context->PSSetShader(renderer->m_pixelShader.Get(), nullptr, 0);

    //renderer->m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    //Material& mat = m_materials[materialIndex];

    //if (mat.HasAlbedo)
    //{
    //    renderer->m_context->PSSetShaderResources(static_cast<UINT>(TextureType::ALBEDO), 1, mat.AlbedoView.GetAddressOf());
    //}
    ///*if (mat.HasNormal)
    //{
    //    renderer->m_context->PSSetShaderResources(static_cast<UINT>(TextureType::NORMAL), 1, mat.NormalView.GetAddressOf());
    //}
    //if (mat.HasMetallicRoughness)
    //{
    //    renderer->m_context->PSSetShaderResources(static_cast<UINT>(TextureType::METALLIC_ROUGHNESS), 1, mat.MetallicRoughnessView.GetAddressOf());
    //}
    //if (mat.HasEmissive)
    //{
    //    renderer->m_context->PSSetShaderResources(static_cast<UINT>(TextureType::EMISSIVE), 1, mat.EmissiveView.GetAddressOf());
    //}
    //if (mat.HasAO)
    //{
    //    renderer->m_context->PSSetShaderResources(static_cast<UINT>(TextureType::AO), 1, mat.AOView.GetAddressOf());
    //}*/
    return true;
}

// this function needs to go. The rendering has to be handled directly in the scene, by passing it to record command buffer.

//void Cravillac::Model::Render()
//{
//    // Bind buffers
//    UINT stride = sizeof(SimpleVertex);
//    UINT offset = 0;
//    renderer->m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
//    renderer->m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
//
//    // Draw each primitive
//    for (const auto& prim : m_primitives)
//    {
//        // Set material textures
//        bool val = SetTexResources(prim.materialIndex);
//
//        if (val)
//        {
//            renderer->m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
//
//            DirectX::XMMATRIX worldMatrix = prim.transform.Matrix;
//
//            UpdateCB(prim, camera);
//
//            // Draw primitive
//            renderer->m_context->DrawIndexed(prim.indexCount, prim.startIndex, prim.startVertex);
//        }
//    }
//}

void Cravillac::Model::ValidateResources() const
{
    Log::Info("[CGLTF] Validating Model Resources:");
    Log::Info("[CGLTF] Vertices: " + std::to_string(m_vertices.size()));
    Log::Info("[CGLTF] Indices: " + std::to_string(m_indices.size()));
    Log::Info("[CGLTF] Materials: " + std::to_string(m_materials.size()));
    Log::Info("[CGLTF] Primitives: " + std::to_string(m_primitives.size()));

    // Check camera position
    DirectX::XMFLOAT3 pos;
    /*XMStoreFloat3(&pos, camera->GetPosition());
    Log::Info("[D3D] Camera Position: " + std::to_string(pos.x) + ", " +
              std::to_string(pos.y) + ", " + std::to_string(pos.z));*/
}