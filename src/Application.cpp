#include "Application.h"
#include "renderer.h"
#include "Texture.h"

namespace VKTest
{
	Application::Application()
	{
		renderer = std::make_shared<Renderer>();
	}
	void Application::Run()
	{
		renderer->Run();
		
		Texture tex1;
		tex1.LoadTexture(renderer, "../../../../assets/textures/texture.jpg");
		textures.push_back(tex1);
		renderer->Submit(textures);

		Render();
	}
	void Application::Render()
	{
		renderer->Render();
	}
	void Application::CleanUp()
	{
		renderer->Cleanup();
	}
	Application::~Application()
	{
		CleanUp();
	}
}