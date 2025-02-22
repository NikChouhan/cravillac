#include <memory>
#include <string>

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
		Model();
		void LoadModel(std::shared_ptr<Renderer> renderer, std::string path);

	private:

	public:

	private:
		std::shared_ptr<Renderer> renderer;
		std::string m_dirPath;	
	};
}