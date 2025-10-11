#pragma once
#include "GfxDevice.h"

struct Pipeline;
struct Texture;

struct DescriptorPool
{
	vk::DescriptorPool _descriptorPool;
};

struct DescriptorPoolDesc
{

	std::vector<vk::DescriptorPoolSize> _poolSizes;
	vk::DescriptorPoolCreateFlagBits _flags;
	u32 _maxSets;
};

struct Descriptor
{
	vk::DescriptorSet _descriptorSet;
};

struct DescriptorDesc
{
	u32 _set;
	u32 _binding;
	vk::DescriptorType _type;
	u32 _descriptorCount;
	vk::ShaderStageFlagBits _stageFlags;
	std::vector<Texture> _textures;
};

DescriptorPool CreateDescriptorPool(GfxDevice& gfxDevice, DescriptorPoolDesc desc);
void DestroyDescriptorPool(GfxDevice& gfxDevice, DescriptorPool& pool);

Descriptor CreateDescriptorSet(GfxDevice gfxDevice, DescriptorPool& pool, Pipeline& pipeline, DescriptorDesc desc);
void DestroyDescriptor(GfxDevice& gfxDevice, Descriptor& descriptor);