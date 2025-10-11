#pragma once
#include "GfxDevice.h"

struct FrameSync
{
	std::vector<vk::Semaphore> _imgAvailableSem;
	std::vector<vk::Semaphore> _renderFinishedSem;
	std::vector<vk::Fence> _inFlightFences;
};

struct FrameSyncDesc
{
	bool _isTrue = true;
};

FrameSync CreateFrameSync(GfxDevice gfxDevice, size_t swapchainTextureCount, FrameSyncDesc desc);