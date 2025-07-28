#pragma once

#include <pch.h>
#include <renderer.h>

namespace CV
{
	struct ImguiRenderer
	{
		void InitImgui(std::shared_ptr<CV::Renderer> renderer, GLFWwindow* _window);
		static void BeginFrame();
		void ImguiFrameRender(GLFWwindow* _window, ImDrawData* _drawData);

		vk::DescriptorPool _imguiDescriptorPool;
		vk::CommandPool _imguiCommandPool;
	};
}
