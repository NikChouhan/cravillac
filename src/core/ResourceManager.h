#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <unordered_map>

#include "vk_utils.h"
#include "BufferBuilder.h"
#include "ImageBuilder.h"

namespace Cravillac
{
	class ResourceManager
	{
	public:
		ResourceManager(VkDevice device, VkPhysicalDevice physicalDevice);
		~ResourceManager();
		VkDevice getDevice() { return m_device; }
		VkPhysicalDevice getPhysicalDevice() { return m_physicalDevice; }
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		// buffer
		BufferBuilder CreateBufferBuilder();
		VkBuffer CreateBuffer(VkDeviceSize size, VkDeviceMemory& outMemory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties); // pass the usage for the vertex/index buffer distinction
		// image
		// TODO
		// ImageBuilder CreateImageBuilder();
		
		// descriptor
		void ConfigureDescriptorPoolSizes(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
		VkDescriptorSet AllocateDescriptorSet(std::vector<VkDescriptorSetLayout>& layout);
		void UpdateDescriptorSet(VkDescriptorSet set);

	private:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;	
		VkDescriptorPool m_descriptorPool;
	};
}

#endif
