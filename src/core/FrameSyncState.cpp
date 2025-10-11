#include "FrameSyncState.h"

FrameSync CreateFrameSync(GfxDevice gfxDevice, size_t swapchainTextureCount, FrameSyncDesc desc)
{
	FrameSync frameSync = {};

    frameSync._imgAvailableSem.resize(MAX_FRAMES_IN_FLIGHT);
    // only renderFinishedSem is resized to swapchain images size because it is used in two queues, it is signalled from graphics queue
    // when render is finished, and waited by present queue. If we index it with current frame parameter which is just cpu side fence parameter
    // it wont work. In the cpu code the current frame parameter is changed in the end of the loop, and its asynchronous to the gpu rendering and
    // present code, so its possible that the current frame parameter changes before the presentation is done. Remember that the current frame
    // var was used to index into the renderFinishedSem, so its possible (and highly likely for gpu driven work, where cpu is more idle than the gpu)
    // for it to index into the wrong semaphore, and signal an already signalled semaphore.
    frameSync._renderFinishedSem.resize(swapchainTextureCount);
    frameSync._inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreCI{};

    vk::FenceCreateInfo fenceCI{};
    fenceCI.flags = vk::FenceCreateFlagBits::eSignaled;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {

        try
        {
            frameSync._imgAvailableSem[i] = gfxDevice._device.createSemaphore(semaphoreCI);
            frameSync._inFlightFences[i] = gfxDevice._device.createFence(fenceCI);

            printl(Log::LogLevel::Info, "[VULKAN] Fence/Semaphore creation Success for frame {} ", std::to_string(i));
        }
        catch (const vk::SystemError& err)
        {
            printl(Log::LogLevel::Error, "[VULKAN] Fence/Semaphore creation Failure for frame {}, error: {}", std::to_string(i), std::string(err.what()));
            throw;
        }
    }
    for (int i = 0; i < swapchainTextureCount; i++)
    {

        try
        {
            frameSync._renderFinishedSem[i] = gfxDevice._device.createSemaphore(semaphoreCI);
            printl(Log::LogLevel::Info, "[VULKAN] Semaphore renderfinished creation success");
        }
        catch (const vk::SystemError& err)
        {
            std::string error = err.what();
            printl(Log::LogLevel::Error, "[VULKAN] Semaphore renderfinished creation failure {}", error);
        }
    }

    return frameSync;
}
