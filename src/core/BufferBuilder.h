#ifndef BUFFER_BUILDER_H
#define BUFFER_BUILDER_H

#include <common.h>

namespace Cravillac
{
	class ResourceManager;

	class BufferBuilder
	{
	public:
		BufferBuilder(ResourceManager& recourceManager);
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

#endif // !BUFFER_BUILDER_H
