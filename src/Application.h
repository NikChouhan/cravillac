#pragma once
#include <vector>
#include <memory>

namespace Cravillac
{
	class Renderer;
	class Texture;

	class Application
	{
	public:
		Application();
		void Run();

	private:
		std::shared_ptr<Renderer> renderer;
		std::vector<Texture> textures;
	public:
	};
}