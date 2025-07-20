#include <pch.h>

#include "BufferBuilder.h"
#include "ResourceManager.h"
#include "Log.h"
#include "vk_utils.h"

namespace CV
{
	BufferBuilder::BufferBuilder(ResourceManager& resourceManager) : _resourceManager(resourceManager) {}

	BufferBuilder& BufferBuilder::setSize(vk::DeviceSize size)
	{
		this->m_size = size;
		return *this;
	}

	BufferBuilder& BufferBuilder::setUsage(vk::BufferUsageFlags usage)
	{
		this->m_usage = usage;
		return *this;
	}

	BufferBuilder& BufferBuilder::setMemoryProperties(vk::MemoryPropertyFlags properties)
	{
		this->m_memProps = properties;
		return *this;
	}

	vk::Buffer BufferBuilder::build(vk::DeviceMemory& outMemory) const {
		const auto device = _resourceManager.getDevice();
		const auto physicalDevice = _resourceManager.getPhysicalDevice();
		vk::BufferCreateInfo bufferCI{};
		bufferCI.size = m_size;
		bufferCI.usage = m_usage;
		//bufferCI.flags = vk::BufferCreateFlagBits::eDeviceAddressCaptureReplay;
		bufferCI.sharingMode = vk::SharingMode::eExclusive;

		vk::Buffer buffer{};
		VK_ASSERT(device.createBuffer(&bufferCI, nullptr, &buffer),
			[&]()
			{
				device.destroyBuffer(buffer);
			});
		vk::MemoryRequirements memRequirements;
		device.getBufferMemoryRequirements(buffer, &memRequirements);

		vk::MemoryAllocateFlagsInfo allocFlagsInfo{};

		auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, m_memProps);

		vk::MemoryAllocateInfo allocInfo = {};
		if (m_usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
			allocFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
			allocInfo.pNext = &allocFlagsInfo;
		}
		else allocInfo.pNext = nullptr;

		allocInfo.allocationSize = memRequirements.size;
		if (memoryTypeIndex.has_value())
			allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, m_memProps).value();
		else printl(Log::LogLevel::Error, "[VULKAN] No memory type found");

		VK_ASSERT(device.allocateMemory(&allocInfo, nullptr, &outMemory),
			[&] ()
			{
				device.destroyBuffer(buffer);
			});

		device.bindBufferMemory(buffer, outMemory, 0);
		if (!buffer) {
			printl(Log::LogLevel::Error,"[BUFFER] Failed to bind memory to buffer");
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, outMemory, nullptr);
			throw std::runtime_error("Failed to bind memory to buffer");
		}

		return buffer;
	};
}
