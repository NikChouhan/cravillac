#pragma once

struct FrameSyncState
{
	vk::Semaphore _imgAvailableSem;
	vk::Semaphore _renderFinishedSem;
	vk::Fence _inFlightFence;
};