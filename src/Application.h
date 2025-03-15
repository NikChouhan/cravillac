#pragma once
#include <vector>
#include <memory>

#include "common.h"

namespace Cravillac
{
	constexpr uint32_t WIDTH = 800;
	constexpr uint32_t HEIGHT = 600;

	class Renderer;
	class Texture;
	class ResourceManager;

	class Application
	{
	public:
		Application(const char* title);
		void Init();
		void Run();
		void DrawFrame();

	private:
		VkSurfaceKHR m_surface;
		std::shared_ptr<Renderer> renderer;
		GLFWwindow* m_window = nullptr;
		ResourceManager* m_resourceManager;
	};
}