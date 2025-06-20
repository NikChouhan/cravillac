#ifndef BUFFER_BUILDER_H
#define BUFFER_BUILDER_H

#include <vulkan/vulkan.hpp>

// have the staging buffers here so no staging buffer copying or such is not visible in the application side

namespace Cravillac
{
	class ResourceManager;

	class BufferBuilder
	{
	public:
		explicit BufferBuilder(ResourceManager& resourceManager);
		~BufferBuilder() = default;

		BufferBuilder& setSize(vk::DeviceSize size);
		BufferBuilder& setUsage(vk::BufferUsageFlags usage);
		BufferBuilder& setMemoryProperties(vk::MemoryPropertyFlags properties);

		vk::Buffer build(vk::DeviceMemory& outMemory) const;

	private:
		ResourceManager& m_resourceManager;
		vk::DeviceSize m_size{};
		vk::BufferUsageFlags m_usage{};
		vk::MemoryPropertyFlags m_memProps{};
	};
}

#endif
