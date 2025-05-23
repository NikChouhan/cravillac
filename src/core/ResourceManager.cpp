#include <vector>

#include "ResourceManager.h"

#include <array>

#include "Log.h"
#include "renderer.h"
#include "vk_utils.h"

namespace Cravillac
{
	ResourceManager::ResourceManager(std::shared_ptr<Renderer> renderer) : m_renderer(renderer), m_descriptorPool(VK_NULL_HANDLE)
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

	VkDevice ResourceManager::getDevice()
	{
		return m_renderer->m_device;
	}

	VkPhysicalDevice ResourceManager::getPhysicalDevice()
	{
		return m_renderer->m_physicalDevice;
	}

	uint32_t ResourceManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_renderer->m_physicalDevice, &memProperties);

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


	void ResourceManager::ConfigureDescriptorPoolSizes(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets)
	{
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_renderer->m_device, m_descriptorPool, nullptr);
		}

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolCI.maxSets = maxSets;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();

		if (vkCreateDescriptorPool(m_renderer->m_device, &poolCI, nullptr, &m_descriptorPool) != VK_SUCCESS)
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

	VkDescriptorSet ResourceManager::CreateDescriptorSet(VkDescriptorSetLayout layout)
	{
		return CreateDescriptorBuilder()
			.allocateDescriptorSet(layout);
	}
	VkDescriptorSet ResourceManager::UpdateDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkDeviceSize size, std::optional<std::vector<Cravillac::Texture>> textures)
	{
		return CreateDescriptorBuilder()
			.updateDescriptorSet(set, binding, type, buffer, size, textures);
	}

	VkShaderModule ResourceManager::getShaderModule(const std::string& shaderPath)
	{
		auto shaderCode = ReadShaderFile(shaderPath);
		return createShaderModule(shaderPath);
	}

	VkDescriptorSetLayout ResourceManager::getDescriptorSetLayout(const std::string& layoutKey)
	{
		if (m_descriptorSetLayoutCache.contains(layoutKey))
		{
			return m_descriptorSetLayoutCache[layoutKey];
		}
		return createDescriptorSetLayout(layoutKey);
	}

	VkShaderModule ResourceManager::createShaderModule(const std::string& shaderPath)
	{
		if (m_shaderModuleCache.contains(shaderPath))
		{
			return m_shaderModuleCache[shaderPath];
		}

		auto shaderCode = ReadShaderFile(shaderPath);

		VkShaderModuleCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shaderCode.size(),
			.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()) };

		VkShaderModule shaderModule;

		auto device = getDevice();

		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			Log::Error("[SHADER] Shader Module creation Failed");
		}
		else
			Log::Info("[SHADER] Shader Module creation Success");
		m_shaderModuleCache[shaderPath] = shaderModule;
		return shaderModule;
	}

	VkDescriptorSetLayout ResourceManager::createDescriptorSetLayout(const std::string& layoutKey)
	{
		// what this does is create a simple layout for the descriptor set. It tells the driver that we will be
		// creating a set with two bindings. One for the ubo buffer and the other for the sampler and inform it about the characteristics of both.
		// so that the validation layers can come in to help with the issues if the actual descriptor set has discrepancy with the layout.
		//

		auto device = getDevice();
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{};
		std::vector<VkDescriptorBindingFlags> bindingFlags;

		if (layoutKey == "ubo") 
		{
			VkDescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = 0;
			uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboBinding.descriptorCount = 1;
			uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(uboBinding);
		}
		else if (layoutKey == "textures") 
		{
			VkDescriptorSetLayoutBinding textureBinding{};
			textureBinding.binding = 0;
			textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			textureBinding.descriptorCount = 98;
			textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			textureBinding.pImmutableSamplers = nullptr;
			bindings.push_back(textureBinding);

			bindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
			bindingFlagsCI = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = 1,
				.pBindingFlags = bindingFlags.data()
			};
		}
		else if (layoutKey == "ssbo")
		{
			VkDescriptorSetLayoutBinding ssboBinding{};
			ssboBinding.binding = 0;
			ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			ssboBinding.descriptorCount = 1;
			ssboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			ssboBinding.pImmutableSamplers = nullptr;
			bindings.push_back(ssboBinding);
		}
		// Add more layoutKey cases or make it configurable via a vector input

		VkDescriptorSetLayoutCreateInfo descLayoutCI{};
		descLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
		descLayoutCI.pBindings = bindings.data();
		if (!bindingFlags.empty()) {
			descLayoutCI.pNext = &bindingFlagsCI;
			descLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		}

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(device, &descLayoutCI, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		m_descriptorSetLayoutCache[layoutKey] = layout;
		return layout;

		//now with the above info for the descriptor set layout, we know that the descriptor set is created with the actual data
		// that we are going to push to it. So if I have multiple models whose data -textures, primitives, indices are to be handled
		// I can't do it at the global renderer level. What I need is the implementation in the model class itself. If I do it here, all the data - tex, primitives,indices
		// must be known to the VkDescriptorWriteInfo. Passing the texture data for every samppler feels foolish
		//
		// (remember I might keep the descriptor set for the ubo buffer as I know it its meant to be for the global renderer, and is not model specific.

	}
};
