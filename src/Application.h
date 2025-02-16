#pragma once
#include <vector>
#include <memory>

namespace VKTest
{
	class Renderer;
	class Texture;

	class Application
	{
	public:
		Application();
		void Run();
		void Render();
		void CleanUp();
		~Application();

	private:
		std::shared_ptr<Renderer> renderer;
		std::vector<Texture> textures;
	public:
	};
}