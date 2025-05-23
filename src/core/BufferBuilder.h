#ifndef BUFFER_BUILDER_H
#define BUFFER_BUILDER_H

#include <common.h>

// have the staging buffers here so no staging buffer copying or such is not visible in the application side

namespace Cravillac
{
	class ResourceManager;

	class BufferBuilder
	{
	public:
		BufferBuilder(ResourceManager& resourceManager);
		~BufferBuilder() {}

		BufferBuilder& setSize(VkDeviceSize size);
		BufferBuilder& setUsage(VkBufferUsageFlags usage);
		BufferBuilder& setMemoryProperties(VkMemoryPropertyFlags properties);

		VkBuffer build(VkDeviceMemory& outMemory);

	private:
		ResourceManager& m_resourceManager;
		VkDeviceSize m_size{};
		VkBufferUsageFlags m_usage{};
		VkMemoryPropertyFlags m_memProps{};
	};
}

#endif
