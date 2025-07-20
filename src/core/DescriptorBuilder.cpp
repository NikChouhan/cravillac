#include <pch.h>

#include "DescriptorBuilder.h"
#include "ResourceManager.h"
#include "Log.h"
#include "Texture.h"

namespace CV 
{
	DescriptorBuilder::DescriptorBuilder(ResourceManager& resourceManager) : _resourceManager(resourceManager){}

	vk::DescriptorSet DescriptorBuilder::allocateDescriptorSet(vk::DescriptorSetLayout layout) const
	{
		vk::DescriptorPool descriptorPool = _resourceManager.getDescriptorPool();
		vk::Device device = _resourceManager.getDevice();

		vk::DescriptorSetAllocateInfo allocInfo{};
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		vk::DescriptorSet descriptorSet;

		VK_ASSERT(device.allocateDescriptorSets(&allocInfo, &descriptorSet));

		return descriptorSet;
	}

	vk::DescriptorSet DescriptorBuilder::updateDescriptorSet(vk::DescriptorSet set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, vk::DeviceSize bufferSize, std::optional<std::vector<CV::Texture>> texturesOpt) const
	{

		// here the binding is for the descriptor in the descriptor set.
		// 0 for ubo , 1 for texture image and both are part of the same m_descriptorSet[frameNumber]
		// refer to YouTube Brendan Galea's descriptor to get an image of what's going on.
		// same set, different bindings for fast access


		auto device = _resourceManager.getDevice();
		if (buffer && (!texturesOpt.has_value()))
		{
			vk::DescriptorBufferInfo descBI{};
			descBI.buffer = buffer;
			descBI.offset = 0;
			descBI.range = bufferSize;

			vk::WriteDescriptorSet uboSet{};
			uboSet.dstSet = set;
			uboSet.dstBinding = binding;
			uboSet.dstArrayElement = 0;
			uboSet.descriptorCount = 1;
			uboSet.descriptorType = type;
			uboSet.pBufferInfo = &descBI;

			device.updateDescriptorSets(1u, &uboSet, 0, nullptr);
		}

		if (!buffer && texturesOpt.has_value())
		{
			// bindless
			std::vector<vk::DescriptorImageInfo> imageInfos{};
			std::vector<Texture>& textures = texturesOpt.value();
			for (auto& tex : textures)
			{
				vk::DescriptorImageInfo info{};
				assert(tex.m_texImage);
				assert(tex.m_texSampler);
				assert(tex.m_texImageView);
				info.sampler = tex.m_texSampler;
				info.imageView = tex.m_texImageView;
				info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

				imageInfos.push_back(info);
			}

			// here the binding is for the descriptor in the descriptor set.
			// 0 for ubo , 1 for texture image and both are part of the same m_descriptorSet[frameNumber]
			// refer to YouTube Brendan Galea's descriptor to get an image of what's going on.
			// same set, different bindings for fast access

			vk::WriteDescriptorSet sampSet{};
			sampSet.dstSet = set;
			sampSet.dstBinding = binding;
			sampSet.dstArrayElement = 0;
			sampSet.descriptorCount = static_cast<uint32_t>(textures.size());
			sampSet.descriptorType = type;
			sampSet.pImageInfo = imageInfos.data();
			device.updateDescriptorSets(1u, &sampSet, 0, nullptr);
		}
		return set;
	}
}
