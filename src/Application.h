#pragma once
#include <memory>

namespace VKTest
{
	class Renderer;


	class Application
	{
	public:
		Application();
		void Run();
		void Render();
		void CleanUp();
		~Application();

	private:
		std::unique_ptr<Renderer> renderer;

	public:
		
	};
}