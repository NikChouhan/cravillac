#include "Application.h"
#include "renderer.h"

namespace VKTest
{
	Application::Application()
	{
		renderer = std::make_unique<Renderer>();
	}
	void Application::Run()
	{
		renderer->Run();
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