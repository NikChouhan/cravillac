#ifndef DESCRIPTOR_BUILDER_H
#define DESCRIPTOR_BUILDER_H

#include <vector>
#include <memory>
#include <optional>

#include <vulkan/vulkan.hpp>

namespace Cravillac
{
	class ResourceManager;
	class Texture;

	class DescriptorBuilder
	{
	public:
		// methods
		DescriptorBuilder(ResourceManager& resourceManager);
		vk::DescriptorSet allocateDescriptorSet(vk::DescriptorSetLayout layout) const;
		vk::DescriptorSet updateDescriptorSet(vk::DescriptorSet set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, vk::DeviceSize bufferSize, std::optional<std::vector<Cravillac::Texture>> textures) const;

		// vars
		ResourceManager& m_resourceManager;
	};
}

#endif