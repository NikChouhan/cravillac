#include <pch.h>

#include <vector>
#include "ResourceManager.h"
#include <array>
#include "Log.h"
#include "renderer.h"
#include "vk_utils.h"


namespace CV
{
	ResourceManager::ResourceManager(const std::shared_ptr<Renderer>& renderer) : _renderer(renderer), m_descriptorPool(VK_NULL_HANDLE)
	{
		pipelineManager = new PipelineManager(this, _renderer);
	}

	ResourceManager::~ResourceManager()
	{
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(_renderer->_device, m_descriptorPool, nullptr);
		}
	}

	vk::Device ResourceManager::getDevice() const
	{
		return _renderer->_device;
	}

	vk::PhysicalDevice ResourceManager::getPhysicalDevice() const
	{
		return _renderer->_physicalDevice;
	}

	BufferBuilder ResourceManager::CreateBufferBuilder()
	{
		return BufferBuilder(*this);
	}


	void ResourceManager::ConfigureDescriptorPoolSizes(const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets)
	{
		if (m_descriptorPool)
		{
			vkDestroyDescriptorPool(_renderer->_device, m_descriptorPool, nullptr);
		}

		vk::DescriptorPoolCreateInfo poolCI{};
		poolCI.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
		poolCI.maxSets = maxSets;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();

		VK_ASSERT_L(_renderer->_device.createDescriptorPool(& poolCI, nullptr,&m_descriptorPool),
			[&]()
			{
				_renderer->_device.destroyDescriptorPool(m_descriptorPool);
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
	vk::DescriptorSet ResourceManager::UpdateDescriptorSet(vk::DescriptorSet set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, vk::DeviceSize size, const std::optional<std::vector<CV::Texture>>& textures)
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
			vk::DescriptorSetLayoutBinding _uboBinding;
			_uboBinding.binding = 0;
			_uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
			_uboBinding.descriptorCount = 1;
			_uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			_uboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(_uboBinding);
		}
		else if (layoutKey == "textures") 
		{
			vk::DescriptorSetLayoutBinding _textureBinding;
			_textureBinding.binding = 0;
			_textureBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			_textureBinding.descriptorCount = MAX_TEXTURES;
			_textureBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
			_textureBinding.pImmutableSamplers = nullptr;
			bindings.push_back(_textureBinding);

			bindingFlags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind);
			bindingFlagsCI.bindingCount = 1;
			bindingFlagsCI.pBindingFlags = bindingFlags.data();
		}
		else if (layoutKey == "ssbo")
		{
			vk::DescriptorSetLayoutBinding _ssboBinding;
			_ssboBinding.binding = 0;
			_ssboBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
			_ssboBinding.descriptorCount = 1;
			_ssboBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
			_ssboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(_ssboBinding);
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
