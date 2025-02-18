#include <memory>

#include <fastgltf/core.hpp>

#include "common.h"

struct Primitive
{

};

namespace VKTest
{
	class Renderer;
}

namespace VKTest
{
	class Model
	{
	public:
		Model() {};
		void LoadModel(std::shared_ptr<Renderer> renderer, const char* path);

	private:

	public:

	private:
		fastgltf::Asset gltfModel;
	};
}