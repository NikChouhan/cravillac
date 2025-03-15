#include "Application.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"
#include "ResourceManager.h"
#include "common.h"

namespace Cravillac
{
	Application::Application(const char* title) : m_resourceManager(nullptr), m_window(nullptr), m_surface(VK_NULL_HANDLE)
	{
		renderer = std::make_shared<Renderer>();

	}
	void Application::Init()
	{
		Log::Init();

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

		renderer->InitVulkan();

		if (glfwCreateWindowSurface(renderer->m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Surface Creation Failure");
		}
		else
			Log::Info("[VULKAN] Surface Creation Success");
		// post surface stuff
		renderer->PickPhysicalDevice();
		renderer->CreateLogicalDevice();
		renderer->CreateSwapChain();
	}
	void Application::Run()
	{
		DrawFrame();
	}
	void Application::DrawFrame()
	{
		
	}
}