#include "Model.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"

#include <optional>

void VKTest::Model::LoadModel(std::shared_ptr<Renderer> renderer, const char* path)
{
	fastgltf::Parser parser;

	auto data = fastgltf::GltfDataBuffer::FromPath(path);
	if (data.error() != fastgltf::Error::None)
	{
		Log::Error("[FASTGLTF] No GlTF in the provided path");
	}
	
	auto asset = parser.loadGltf(data.get(), path, fastgltf::Options::None);

	if (asset.error() != fastgltf::Error::None)
	{
		Log::Error("[FASTGLTF] Can't parse the GlTF");
	}
	

}
