#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H


#include "common.h"
#include "BufferBuilder.h"
#include "ImageBuilder.h"
#include "DescriptorBuilder.h"

namespace Cravillac
{
	class ResourceManager
	{
	public:
		ResourceManager(VkDevice device, VkPhysicalDevice physicalDevice);
		~ResourceManager();
		VkDevice getDevice() { return m_device; }
		VkPhysicalDevice getPhysicalDevice() { return m_physicalDevice; }
		VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		// buffer
		BufferBuilder CreateBufferBuilder();
		VkBuffer CreateBuffer(VkDeviceSize size, VkDeviceMemory& outMemory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties); // pass the usage for the vertex/index buffer distinction
		// image
		// TODO
		// ImageBuilder CreateImageBuilder();
		
		// descriptor
		void ConfigureDescriptorPoolSizes(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
		DescriptorBuilder CreateDescriptorBuilder();
		VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout& layout);
		VkDescriptorSet UpdateDescriptorSet(VkDescriptorSet& set);

	private:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;	
		VkDescriptorPool m_descriptorPool;
	};
}

#endif
