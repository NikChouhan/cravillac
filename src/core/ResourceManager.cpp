#include <vector>

#include "ResourceManager.h"
#include "Log.h"

namespace Cravillac
{
	ResourceManager::ResourceManager(VkDevice device, VkPhysicalDevice physicalDevice) : m_device(device), m_physicalDevice(physicalDevice), m_descriptorPool(VK_NULL_HANDLE)
	{

	}

	ResourceManager::~ResourceManager()
	{
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
		}
	}

	uint32_t ResourceManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		Log::Error("[MEMORY] Failed to find suitable memory type");
	}

	BufferBuilder ResourceManager::CreateBufferBuilder()
	{
		return BufferBuilder(*this);
	}

	VkBuffer ResourceManager::CreateBuffer(VkDeviceSize size, VkDeviceMemory& outMemory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		return CreateBufferBuilder()
			.setSize(size)
			.setUsage(usage)
			.setMemoryProperties(properties)
			.build(outMemory);
	}

	void ResourceManager::ConfigureDescriptorPoolSizes(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets)
	{
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
		}

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.maxSets = maxSets;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();

		if (vkCreateDescriptorPool(m_device, &poolCI, nullptr, &m_descriptorPool) != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Descriptor Pool creation Failed");
		}
		else
		{
			Log::Info("[VULKAN] Descriptor Pool creation Success");
		}
	}

	DescriptorBuilder ResourceManager::CreateDescriptorBuilder()
	{
		return DescriptorBuilder(*this);
	}

	VkDescriptorSet ResourceManager::CreateDescriptorSet(VkDescriptorSetLayout& layout)
	{
		return CreateDescriptorBuilder()
			.allocateDescriptorSet(layout);
	}
	VkDescriptorSet ResourceManager::UpdateDescriptorSet(VkDescriptorSet& set)
	{
		return CreateDescriptorBuilder()
			.updateDescriptorSet(set);
	}
};
