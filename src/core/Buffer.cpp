#include "Buffer.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.hpp>

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc)
{
	assert(desc._byteSize > 0);

	if (desc._pContents)
	{
		desc._usage |= vk::BufferUsageFlagBits::eTransferDst;
	}

	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.size = desc._byteSize;
	bufferCreateInfo.usage = desc._usage;
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	vma::AllocationCreateInfo allocationCreateInfo;
	allocationCreateInfo.usage = vma::MemoryUsage::eAuto;

	if (desc._access == MemoryAccess::HOST)
	{
		allocationCreateInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
	}

	Buffer buffer = { ._byteSize = desc._byteSize };


	VK_ASSERT(gfxDevice._allocator.createBuffer(&bufferCreateInfo, 
		&allocationCreateInfo, &buffer._resource, &buffer._allocation, nullptr));

	if (desc._access == MemoryAccess::HOST)
	{
		// Persistently mapped memory.
		vmaMapMemory(gfxDevice._allocator, buffer._allocation, &buffer._pMappedData);
	}

	if (desc._pContents)
	{
		if (desc._access == MemoryAccess::HOST)
		{
			memcpy(buffer._pMappedData, desc._pContents, buffer._byteSize);
		}
		else
		{
			Buffer stagingBuffer = CreateBuffer(gfxDevice, {
				._byteSize = desc._byteSize,
				._access = MemoryAccess::HOST,
				._usage = vk::BufferUsageFlagBits::eTransferSrc,
				._pContents = desc._pContents });

			ImmediateSubmit(gfxDevice, [&](const vk::CommandBuffer commandBuffer)
				{
					vk::BufferCopy copyRegion;
					copyRegion.size = desc._byteSize;
					commandBuffer.copyBuffer(stagingBuffer._resource, buffer._resource, 1, &copyRegion);
				});

			DestroyBuffer(gfxDevice, stagingBuffer);
		}
	}
	return buffer;
}

void DestroyBuffer(const GfxDevice& gfxDevice, const Buffer& buffer)
{
	if (buffer._pMappedData)
	{
		vmaUnmapMemory(gfxDevice._allocator, buffer._allocation);
	}
	vmaDestroyBuffer(gfxDevice._allocator, buffer._resource, buffer._allocation);
}

void BufferBarrier(const vk::CommandBuffer commandBuffer, const GfxDevice& gfxDevice, const Buffer& buffer, const vk::AccessFlags srcAccessMask,
                   vk::AccessFlags dstAccessMask, const vk::PipelineStageFlagBits srcStageMask,
                   vk::PipelineStageFlagBits dstStageMask)
{
	vk::BufferMemoryBarrier bufferMemoryBarrier;
	bufferMemoryBarrier.buffer = buffer._resource;
	bufferMemoryBarrier.size = buffer._byteSize;
	bufferMemoryBarrier.srcAccessMask = srcAccessMask;
	bufferMemoryBarrier.dstAccessMask = dstAccessMask;
	bufferMemoryBarrier.srcQueueFamilyIndex = gfxDevice._graphicsQueue._index;
	bufferMemoryBarrier.dstQueueFamilyIndex = gfxDevice._graphicsQueue._index;

	commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, static_cast<vk::DependencyFlags>(0), 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

void FillBuffer(const vk::CommandBuffer commandBuffer, const GfxDevice& gfxDevice, const Buffer& buffer, u32 value,
                vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::PipelineStageFlagBits srcStageMask,
                vk::PipelineStageFlagBits dstStageMask)
{
	BufferBarrier(commandBuffer, gfxDevice, buffer, srcAccessMask, vk::AccessFlagBits::eTransferWrite, srcStageMask,
		vk::PipelineStageFlagBits::eTransfer);

	commandBuffer.fillBuffer(buffer._resource, 0, buffer._byteSize, value);

	BufferBarrier(commandBuffer, gfxDevice, buffer, vk::AccessFlagBits::eTransferWrite, dstAccessMask,
	              vk::PipelineStageFlagBits::eTransfer, dstStageMask);
}
