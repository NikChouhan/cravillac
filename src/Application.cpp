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

		Texture tex{renderer};
		tex.LoadTexture("../../../../assets/textures/texture.jpg");

		renderer->Submit(tex);

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