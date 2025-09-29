#pragma once
#include "GfxDevice.h"

struct FrameSync
{
	vk::Semaphore _imgAvailableSem;
	vk::Semaphore _renderFinishedSem;
	vk::Fence _inFlightFence;
};

struct FrameSyncDesc
{
	bool _isTrue = true;
};

FrameSync CreateFrameSync(GfxDevice gfxDevice, FrameSyncDesc desc);