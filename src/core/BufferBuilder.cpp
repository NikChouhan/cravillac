#include <pch.h>

#include "BufferBuilder.h"
#include "ResourceManager.h"
#include "Log.h"
#include "vk_utils.h"

namespace Cravillac
{
	BufferBuilder::BufferBuilder(ResourceManager& resourceManager) : m_resourceManager(resourceManager) {}

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
		const auto device = m_resourceManager.getDevice();
		const auto physicalDevice = m_resourceManager.getPhysicalDevice();
		vk::BufferCreateInfo bufferCI{};
		bufferCI.size = m_size;
		bufferCI.usage = m_usage;
		bufferCI.sharingMode = vk::SharingMode::eExclusive;

		vk::Buffer buffer{};
		auto result = device.createBuffer(& bufferCI, nullptr, & buffer);

		VK_ASSERT(device.createBuffer(&bufferCI, nullptr, &buffer),
			[&]()
			{
				device.destroyBuffer(buffer);
			});
		vk::MemoryRequirements memRequirements;
		device.getBufferMemoryRequirements(buffer, &memRequirements);
		// TODO: want to have it optional such that only vertex and index buffers use it,
		// but prolly keeping it since thats all where I use it until now
		vk::MemoryAllocateFlagsInfo allocFlagsInfo{};
		allocFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

		auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, m_memProps);

		vk::MemoryAllocateInfo allocInfo = {};
		allocInfo.pNext = &allocFlagsInfo;
		allocInfo.allocationSize = memRequirements.size;
		if (memoryTypeIndex.has_value())
			allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, m_memProps).value();
		else Log::Error("[VULKAN] No memory type found");

		VK_ASSERT(device.allocateMemory(&allocInfo, nullptr, &outMemory),
			[&] ()
			{
				device.destroyBuffer(buffer);
			});

		device.bindBufferMemory(buffer, outMemory, 0);
		if (!buffer) {
			Log::Error("[BUFFER] Failed to bind memory to buffer");
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, outMemory, nullptr);
			throw std::runtime_error("Failed to bind memory to buffer");
		}

		return buffer;
	};
}
