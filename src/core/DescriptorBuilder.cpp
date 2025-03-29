#include "DescriptorBuilder.h"
#include "ResourceManager.h"
#include "Log.h"
#include "Texture.h"



namespace Cravillac 
{
	DescriptorBuilder::DescriptorBuilder(ResourceManager& resourceManager) : m_resourceManager(resourceManager){}

	VkDescriptorSet DescriptorBuilder::allocateDescriptorSet(VkDescriptorSetLayout layout)
	{
		VkDescriptorPool descriptorPool = m_resourceManager.getDescriptorPool();
		VkDevice device = m_resourceManager.getDevice();
		if (descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 2;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSet;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Descriptor Sets creation Failed");
		}
		else
			Log::Info("[VULKAN] Descriptor Sets creation Success");
		return descriptorSet;
	}

	VkDescriptorSet DescriptorBuilder::updateDescriptorSet(uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkDeviceSize bufferSize, std::vector<Cravillac::Texture>* textures)
	{

		// here the binding is for the descriptor in the descriptor set.
		// 0 for ubo , 1 for texture image and both are part of the same m_descriptorSet[frameNumber]
		// refer to YouTube Brendan Galea's descriptor to get an image of what's going on.
		// same set, different bindings for fast access

		VkDescriptorSet set{};
		auto device = m_resourceManager.getDevice();
		if (buffer != VK_NULL_HANDLE && (textures == nullptr && textures->empty()))
		{
			VkDescriptorBufferInfo descBI{};
			descBI.buffer = buffer;
			descBI.offset = 0;
			descBI.range = bufferSize;

			VkWriteDescriptorSet uboSet
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = set,
				.dstBinding = binding,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = type,
				.pBufferInfo = &descBI
			};

			vkUpdateDescriptorSets(device, 1, &uboSet, 0, nullptr);
		}

		if (buffer == VK_NULL_HANDLE && (textures != nullptr && !textures->empty()))
		{
			// bindless
			std::vector<VkDescriptorImageInfo> imageInfos{};

			for (auto tex : *textures)
			{
				VkDescriptorImageInfo info{};
				assert(tex.m_texImage);
				assert(tex.m_texSampler);
				assert(tex.m_texImageView);
				info.sampler = tex.m_texSampler;
				info.imageView = tex.m_texImageView;
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageInfos.push_back(info);
			}

			// here the binding is for the descriptor in the descriptor set.
			// 0 for ubo , 1 for texture image and both are part of the same m_descriptorSet[frameNumber]
			// refer to YouTube Brendan Galea's descriptor to get an image of what's going on.
			// same set, different bindings for fast access

			VkWriteDescriptorSet sampSet
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = set,
				.dstBinding = binding,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(textures->size()),
				.descriptorType = type,
				.pImageInfo = imageInfos.data() 
			};
			vkUpdateDescriptorSets(device, 1, &sampSet, 0, nullptr);
		}
		return set;
	}
}
