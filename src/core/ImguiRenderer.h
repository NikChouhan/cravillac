#pragma once

#include <pch.h>

#include "GfxDevice.h"

namespace CV
{
	struct ImguiRenderer
	{
		void InitImgui(GfxDevice& gfxDevice, GLFWwindow* _window);
		static void BeginFrame();
		void ImguiFrameRender(GLFWwindow* _window, ImDrawData* _drawData);

		vk::DescriptorPool _imguiDescriptorPool;
		vk::CommandPool _imguiCommandPool;
	};
}
