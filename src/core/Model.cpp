#include "Model.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"
#include "vk_utils.h"

#include <string>
namespace Cravillac
{
	Model::Model()
	{
	}

	void Model::LoadModel(std::shared_ptr<Renderer> renderer, std::string path)
	{
		this->renderer = renderer;

		cgltf_options options = {};
		cgltf_data *data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
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
				ProcessNode(scene->nodes[i], data, transform);
			}
			// no of nodes
			Log::InfoDebug("[CGLTF] No of nodes in the scene: ", scene->nodes_count);

            SetVertIndBuffers();

			Log::Info("[CGLTF] Successfully loaded gltf file");

			//ValidateBuffers();

			cgltf_free(data);
		}
	}

    void Model::ProcessNode(cgltf_node* node, const cgltf_data* data, Transformation& parentTransform)
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

                ProcessPrimitive(&node->mesh->primitives[i], data, localTransform);
            }
        }

        // Recursively process child nodes
        for (size_t i = 0; i < node->children_count; i++)
        {
            ProcessNode(node->children[i], data, localTransform);
        }
    }

    void Model::ProcessPrimitive(cgltf_primitive* primitive, const cgltf_data* data, Transformation& parentTransform)
    {
        uint32_t vertexOffset = vertices.size();
        uint32_t indexOffset = indices.size();

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

        // Get attributes
        cgltf_attribute* pos_attribute = nullptr;
        cgltf_attribute* tex_attribute = nullptr;
        cgltf_attribute* norm_attribute = nullptr;

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

        int vertexCount = pos_attribute->data->count;
        int indexCount = primitive->indices->count;

        for (int i = 0; i < vertexCount; i++)
        {
            SimpleVertex vertex = {};

            // Read original vertex data
            if (cgltf_accessor_read_float(pos_attribute->data, i, &vertex.Pos.x, 3) == 0)
            {
                Log::Warn("[CGLTF] Unable to read Position attributes!");
            }
            if (cgltf_accessor_read_float(tex_attribute->data, i, &vertex.TexCoord.x, 2) == 0)
            {
                Log::Warn("[CGLTF] Unable to read Texture attributes!");
            }
            if (cgltf_accessor_read_float(norm_attribute->data, i, &vertex.Normal.x, 3) == 0)
            {
                Log::Warn("[CGLTF] Unable to read Normal attributes!");
            }

            m_vertices.push_back(vertex);
        }

        for (int i = 0; i < indexCount; i++)
        {
            m_indices.push_back(cgltf_accessor_read_index(primitive->indices, i));
        }

        // TODO: Material later
        // material

        //cgltf_material* material = primitive->material; //remember this is pointing towards the material pointer. In gltf as long as the material remains the same, the pointer is the same even for diff primitives

        //if (materialLookup.find(material) == materialLookup.end())
        //{
        //    Material mat = {};

        //    HRESULT hr = E_FAIL;

        //    // map texture types to their respective textures
        //    std::unordered_map<TextureType, cgltf_texture_view*> textureMap;

        //    //the following code for materials is very much unoptimised. It should only look for materials once, make a texture, sampler and save it in a map, not per primitive
        //    //#TODO

        //    if (material->has_pbr_metallic_roughness)
        //    {
        //        cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
        //        // Map base color texture (albedo)
        //        if (pbr->base_color_texture.texture)
        //        {
        //            textureMap[TextureType::ALBEDO] = &pbr->base_color_texture;
        //        }
        //        if (pbr->metallic_roughness_texture.texture)
        //        {
        //            // Map metallic-roughness texture
        //            textureMap[TextureType::METALLIC_ROUGHNESS] = &pbr->metallic_roughness_texture;
        //            //mat.MaterialName = 
        //        }
        //    }

        //    if (material->normal_texture.texture)
        //    {
        //        // Map normal texture
        //        textureMap[TextureType::NORMAL] = &material->normal_texture;
        //    }
        //    if (material->emissive_texture.texture)
        //    {
        //        // Map emissive texture
        //        textureMap[TextureType::EMISSIVE] = &material->emissive_texture;
        //    }

        //    if (material->occlusion_texture.texture)
        //    {
        //        // Map occlusion texture
        //        textureMap[TextureType::AO] = &material->occlusion_texture;
        //    }
        //    if (material->has_pbr_specular_glossiness)
        //    {
        //        cgltf_pbr_specular_glossiness* pbr = &material->pbr_specular_glossiness;
        //        if (pbr->specular_glossiness_texture.texture)
        //        {
        //            //textureMap[TextureType::SPECULAR_GLOSSINESS] = &material->tex
        //        }
        //    }

        //    // Load all textures from the map if they haven't been loaded before
        //    for (const auto& [type, view] : textureMap)
        //    {
        //        static int i = 0;
        //        std::string textureIdentifier = std::to_string(static_cast<int>(type)); // Unique identifier for texture type
        //        std::string imageName = view->texture->image->uri;
        //        std::string pathTexture = imageName;

        //        if (loadedTextures.find(pathTexture) == loadedTextures.end()) // If not already loaded
        //        {
        //            hr = LoadMaterialTexture(mat, view, type);
        //            if (FAILED(hr))
        //            {
        //                Log::Error("[Texture] Failed to load texture of type " + textureIdentifier + " with name: " + imageName);
        //                Log::InfoDebug("  #", i++);
        //            }
        //            else
        //            {
        //                Log::Info("[Texture] Loaded texture of type " + textureIdentifier + " with name: " + imageName);
        //                Log::InfoDebug("  #", i++);
        //                loadedTextures.insert(imageName); // Mark this texture type as loaded
        //            }
        //        }
        //        else
        //        {
        //            Log::Info("[Texture] Skipping already loaded texture of type " + textureIdentifier + " with name: " + imageName);
        //        }
        //    }
        //    m_materials.push_back(mat);
        //    materialLookup[material] = m_materials.size() - 1;
        //    prim.materialIndex = materialLookup[material];
        //}

        //prim.materialIndex = materialLookup[material];
        prim.transform = parentTransform;
        prim.startIndex = indexOffset;
        prim.startVertex = vertexOffset;
        prim.vertexCount = vertexCount;
        prim.indexCount = indexCount;

        m_primitives.push_back(prim);
    }

    void Model::SetVertIndBuffers()
    {
        // vertex buffer
        VkDeviceSize vertBufferSize = sizeof(SimpleVertex) * m_vertices.size();

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        CreateBuffer(renderer->m_device, renderer->m_physicalDevice, vertBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // What map does is : "When you call vkMapMemory, what gets mapped is a region of GPU memory (allocated via vkAllocateMemory) into the CPU's address space. This allows the CPU to directly access that GPU memory as if it were regular RAM."
        // the void data* (its stored in CPU address space or RAM (or caches and bla bla but for CPU remember) and it stores the gpu memory pointer(?) and is mapping of the GPU memory where the contents are copied
        void* data{};

        // imagine vkmapmemory as a function that takes in gpu memory and cpu memory pointer, map the memory and return a cpu pointer (void data*) to be used to copy stuf into it, which due to its mappingis also stored in the gpu memory
        vkMapMemory(renderer->m_device, stagingBufferMemory, 0, vertBufferSize, 0, &data);
        memcpy(data, m_vertices.data(), size_t(vertBufferSize));

        // (this releases and frees the data pointer implicitly after unmapping it ?)
        vkUnmapMemory(renderer->m_device, stagingBufferMemory);

        CreateBuffer(renderer->m_device, renderer->m_physicalDevice, vertBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

        CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_vertexBuffer, vertBufferSize);

        vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
        vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

        // index buffer

        VkDeviceSize indBufferSize = sizeof(m_indices[0]) * m_indices.size();

        VkBuffer stagingBuffer{};
        VkDeviceMemory staginGBufferMem{};
        CreateBuffer(renderer->m_device, renderer->m_physicalDevice, indBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginGBufferMem);

        void* data{};
        vkMapMemory(renderer->m_device, staginGBufferMem, 0, indBufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)indBufferSize);
        vkUnmapMemory(renderer->m_device, staginGBufferMem);

        CreateBuffer(renderer->m_device, renderer->m_physicalDevice, indBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

        CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_indexBuffer, indBufferSize);

        vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
        vkFreeMemory(renderer->m_device, staginGBufferMem, nullptr);
    }



} 
