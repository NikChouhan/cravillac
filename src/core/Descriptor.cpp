#include "Descriptor.h"

#include "Pipeline.h"
#include "Texture.h"

DescriptorPool CreateDescriptorPool(GfxDevice& gfxDevice, DescriptorPoolDesc desc)
{
	DescriptorPool descriptorPool;

	vk::DescriptorPoolCreateInfo poolCI{};
	poolCI.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
	poolCI.maxSets = desc._maxSets;
	poolCI.poolSizeCount = static_cast<uint32_t>(desc._poolSizes.size());
	poolCI.pPoolSizes = desc._poolSizes.data();

	VK_ASSERT(gfxDevice._device.createDescriptorPool(&poolCI, nullptr, &descriptorPool._descriptorPool));

	return descriptorPool;
}

Descriptor CreateDescriptorSet(GfxDevice gfxDevice, DescriptorPool& descriptorPool, Pipeline& pipeline, DescriptorDesc desc)
{
	Descriptor descriptorSet;

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool._descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &pipeline._setLayout;

	VK_ASSERT(gfxDevice._device.allocateDescriptorSets(&allocInfo, &descriptorSet._descriptorSet));

	std::vector<vk::DescriptorImageInfo> imageInfos{};
	for (auto& tex : desc._textures)
	{
		vk::DescriptorImageInfo info{};
		assert(tex._resource);
		assert(tex._sampler._resource);
		assert(tex._imageView);
		info.sampler = tex._sampler._resource;
		info.imageView = tex._imageView;
		info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		imageInfos.push_back(info);
	}

	vk::WriteDescriptorSet sampSet{};
	sampSet.dstSet = descriptorSet._descriptorSet;
	sampSet.dstBinding = desc._binding;
	sampSet.dstArrayElement = 0;
	sampSet.descriptorCount = static_cast<uint32_t>(desc._textures.size());
	sampSet.descriptorType = desc._type;
	sampSet.pImageInfo = imageInfos.data();
	gfxDevice._device.updateDescriptorSets(1u, &sampSet, 0, nullptr);

	return descriptorSet;
}
