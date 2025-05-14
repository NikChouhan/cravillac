#include "BufferBuilder.h"
#include "ResourceManager.h"
#include "Log.h"

namespace Cravillac
{
	BufferBuilder::BufferBuilder(ResourceManager& resourceManager) : m_resourceManager(resourceManager) {}

	BufferBuilder& BufferBuilder::setSize(VkDeviceSize size)
	{
		this->m_size = size;
		return *this;
	}

	BufferBuilder& BufferBuilder::setUsage(VkBufferUsageFlags usage)
	{
		this->m_usage = usage;
		return *this;
	}

	BufferBuilder& BufferBuilder::setMemoryProperties(VkMemoryPropertyFlags properties)
	{
		this->m_memProps = properties;
		return *this;
	}

	VkBuffer BufferBuilder::build(VkDeviceMemory& outMemory)
	{
		auto device = m_resourceManager.getDevice();
		VkBufferCreateInfo bufferCI{};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.size = m_size;
		bufferCI.usage = m_usage;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer buffer{};
		auto result = vkCreateBuffer(device, &bufferCI, nullptr, &buffer);
		if (result != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Failed to create buffer");
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_resourceManager.FindMemoryType(memRequirements.memoryTypeBits, m_memProps);

		result = vkAllocateMemory(device, &allocInfo, nullptr, &outMemory);
		if (result != VK_SUCCESS) {
			Log::Error("[BUFFER] Failed to allocate memory for buffer");
			vkDestroyBuffer(device, buffer, nullptr);
			throw std::runtime_error("Failed to allocate memory for buffer");
		}

		result = vkBindBufferMemory(device, buffer, outMemory, 0);
		if (result != VK_SUCCESS) {
			Log::Error("[BUFFER] Failed to bind memory to buffer");
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, outMemory, nullptr);
			throw std::runtime_error("Failed to bind memory to buffer");
		}

		return buffer;
	};
}
