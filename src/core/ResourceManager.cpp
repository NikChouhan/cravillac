#include <pch.h>

#include <vector>
#include "ResourceManager.h"
#include <array>
#include "Log.h"
#include "renderer.h"
#include "vk_utils.h"

namespace Cravillac
{
	ResourceManager::ResourceManager(const std::shared_ptr<Renderer>& renderer) : m_renderer(renderer), m_descriptorPool(VK_NULL_HANDLE)
	{
		pipelineManager = new PipelineManager(this, m_renderer);
	}

	ResourceManager::~ResourceManager()
	{
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_renderer->m_device, m_descriptorPool, nullptr);
		}
	}

	vk::Device ResourceManager::getDevice() const
	{
		return m_renderer->m_device;
	}

	vk::PhysicalDevice ResourceManager::getPhysicalDevice() const
	{
		return m_renderer->m_physicalDevice;
	}

	BufferBuilder ResourceManager::CreateBufferBuilder()
	{
		return BufferBuilder(*this);
	}


	void ResourceManager::ConfigureDescriptorPoolSizes(const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets)
	{
		if (m_descriptorPool)
		{
			vkDestroyDescriptorPool(m_renderer->m_device, m_descriptorPool, nullptr);
		}

		vk::DescriptorPoolCreateInfo poolCI{};
		poolCI.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
		poolCI.maxSets = maxSets;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();

		VK_ASSERT_L(m_renderer->m_device.createDescriptorPool(& poolCI, nullptr,&m_descriptorPool),
			[&]()
			{
				m_renderer->m_device.destroyDescriptorPool(m_descriptorPool);
			});
	}

	DescriptorBuilder ResourceManager::CreateDescriptorBuilder()
	{
		return DescriptorBuilder(*this);
	}

	vk::DescriptorSet ResourceManager::CreateDescriptorSet(vk::DescriptorSetLayout layout)
	{
		return CreateDescriptorBuilder()
			.allocateDescriptorSet(layout);
	}
	vk::DescriptorSet ResourceManager::UpdateDescriptorSet(vk::DescriptorSet set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, vk::DeviceSize size, const std::optional<std::vector<Cravillac::Texture>>& textures)
	{
		return CreateDescriptorBuilder()
			.updateDescriptorSet(set, binding, type, buffer, size, textures);
	}

	vk::ShaderModule ResourceManager::getShaderModule(const std::string& shaderPath)
	{
		auto shaderCode = ReadShaderFile(shaderPath);
		return createShaderModule(shaderPath);
	}

	vk::DescriptorSetLayout ResourceManager::getDescriptorSetLayout(const std::string& layoutKey)
	{
		if (m_descriptorSetLayoutCache.contains(layoutKey))
		{
			return m_descriptorSetLayoutCache[layoutKey];
		}
		return createDescriptorSetLayout(layoutKey);
	}

	vk::ShaderModule ResourceManager::createShaderModule(const std::string& shaderPath)
	{
		if (m_shaderModuleCache.contains(shaderPath))
		{
			return m_shaderModuleCache[shaderPath];
		}

		auto shaderCode = ReadShaderFile(shaderPath);

		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		vk::ShaderModule shaderModule;

		auto device = getDevice();

		VK_ASSERT(device.createShaderModule(&createInfo, nullptr, &shaderModule));
		m_shaderModuleCache[shaderPath] = shaderModule;
		return shaderModule;
	}

	vk::DescriptorSetLayout ResourceManager::createDescriptorSetLayout(const std::string& layoutKey)
	{
		// what this does is create a simple layout for the descriptor set. It tells the driver that we will be
		// creating a set with two bindings. One for the ubo buffer and the other for the sampler and inform it about the characteristics of both.
		// so that the validation layers can come in to help with the issues if the actual descriptor set has discrepancy with the layout.
		//

		auto device = getDevice();
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{};
		std::vector<vk::DescriptorBindingFlags> bindingFlags;

		if (layoutKey == "ubo") 
		{
			vk::DescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = 0;
			uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
			uboBinding.descriptorCount = 1;
			uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			uboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(uboBinding);
		}
		else if (layoutKey == "textures") 
		{
			vk::DescriptorSetLayoutBinding textureBinding{};
			textureBinding.binding = 0;
			textureBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			textureBinding.descriptorCount = MAX_TEXTURES;
			textureBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
			textureBinding.pImmutableSamplers = nullptr;
			bindings.push_back(textureBinding);

			bindingFlags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind);
			bindingFlagsCI.bindingCount = 1;
			bindingFlagsCI.pBindingFlags = bindingFlags.data();
		}
		else if (layoutKey == "ssbo")
		{
			vk::DescriptorSetLayoutBinding ssboBinding{};
			ssboBinding.binding = 0;
			ssboBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
			ssboBinding.descriptorCount = 1;
			ssboBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
			ssboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(ssboBinding);
		}
		// Add more layoutKey cases or make it configurable via a vector input

		vk::DescriptorSetLayoutCreateInfo descLayoutCI{};
		descLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
		descLayoutCI.pBindings = bindings.data();
		if (!bindingFlags.empty()) {
			descLayoutCI.pNext = &bindingFlagsCI;
			descLayoutCI.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
		}

		vk::DescriptorSetLayout layout;
		VK_ASSERT(device.createDescriptorSetLayout(&descLayoutCI, nullptr, &layout));

		m_descriptorSetLayoutCache[layoutKey] = layout;
		return layout;

		//now with the above info for the descriptor set layout, we know that the descriptor set is created with the actual data
		// that we are going to push to it. So if I have multiple models whose data -textures, primitives, indices are to be handled
		// I can't do it at the global renderer level. What I need is the implementation in the model class itself. If I do it here, all the data - tex, primitives,indices
		// must be known to the vk::DescriptorWriteInfo. Passing the texture data for every samppler feels foolish
		//
		// (remember I might keep the descriptor set for the ubo buffer as I know it its meant to be for the global renderer, and is not model specific.

	}
};
