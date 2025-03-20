#ifndef DESCRIPTOR_BUILDER_H
#define DESCRIPTOR_BUILDER_H

#include <vector>
#include <memory>
#include "common.h"

namespace Cravillac
{
	class ResourceManager;
	class Texture;

	class DescriptorBuilder
	{
	public:
		// methods
		DescriptorBuilder(ResourceManager& resourceManager);
		VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout& layouts);
		VkDescriptorSet updateDescriptorSet(VkDescriptorSet& set, uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkDeviceSize bufferSize, std::vector<Cravillac::Texture>* textures);

		// vars
		ResourceManager& m_resourceManager;
	};
}

#endif