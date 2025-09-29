#include "FrameSyncState.h"

FrameSync CreateFrameSync(GfxDevice gfxDevice, FrameSyncDesc desc)
{
	FrameSync frameSync = {};

	vk::SemaphoreCreateInfo semaphoreCreateInfo;

	VK_ASSERT(gfxDevice._device.createSemaphore(&semaphoreCreateInfo, nullptr, &frameSync._imgAvailableSem));
	VK_ASSERT(gfxDevice._device.createSemaphore(&semaphoreCreateInfo, nullptr, &frameSync._renderFinishedSem));

	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	VK_ASSERT(gfxDevice._device.createFence(&fenceCreateInfo, nullptr, &frameSync._inFlightFence));

	return frameSync;
}
