#include "Model.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"

#include <cgltf.h>

#include <string>
namespace VKTest
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
				// Transformation transform;
				// ProcessNode(scene->nodes[i], data, m_vertices, m_indices, transform);
			}
			// no of nodes
			Log::InfoDebug("[CGLTF] No of nodes in the scene: ", scene->nodes_count);

			//SetBuffers();

			Log::Info("[CGLTF] Successfully loaded gltf file");

			//ValidateBuffers();

			cgltf_free(data);
		}
	}

} 
