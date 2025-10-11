#pragma once
#include "FrameSyncState.h"
#include "GfxDevice.h"
#include "Texture.h"

struct Swapchain
{
	vk::SwapchainKHR _swapchain;
	vk::Format _format;
	vk::PresentModeKHR _presentMode;
	vk::Extent2D _extent;
	std::vector<Texture> _textures;
};

struct SwapchainDesc
{
	bool _vsyncOption = true;
	u32 _preferresSWImageCount = 2;
	vk::SwapchainKHR _oldSwapchain = nullptr;
};

Swapchain CreateSwapchain(GfxDevice& gfxDevice, GLFWwindow* window, SwapchainDesc desc);
void DestroySwapchain(GfxDevice& gfxDevice, Swapchain& swapchain);
void SubmitAndPresent(GfxDevice& gfxDevice, vk::CommandBuffer& commandBuffer, Swapchain& swapchain, u32 index,
                      FrameSync& frameSyncState);