#pragma once
#include "GfxDevice.h"

struct Buffer
{
	vk::DeviceSize _byteSize{ 0 };
	vk::Buffer _resource = nullptr;
	VmaAllocation _allocation = nullptr;
	void* _pMappedData = nullptr;
};

enum class MemoryAccess : u8
{
	HOST,
	DEVICE
};

struct BufferDesc
{
	u64 _byteSize = 0;                            // Buffer size in bytes
	MemoryAccess _access = MemoryAccess::DEVICE;  // Buffer memory access
	vk::BufferUsageFlags _usage;				  // Buffer usage flags
	void* _pContents = nullptr;                   // [Optional] A buffer can be created with a fixed size but no contents (imagine shader reflection system,
												  // a change in GPU side code is reflected CPU side. The pContents is null created but mapped all the time (?Resizable bar)
};

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc);
void DestroyBuffer(const GfxDevice& gfxDevice, const Buffer& buffer);
void BufferBarrier(vk::CommandBuffer commandBuffer, const GfxDevice& gfxDevice, const Buffer& buffer,
                   vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                   vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
                   vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);

void FillBuffer(vk::CommandBuffer commandBuffer, const GfxDevice& gfxDevice, const Buffer& buffer, u32 value,
                vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
