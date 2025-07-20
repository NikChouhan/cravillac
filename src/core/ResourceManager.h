#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "common.h"
#include "BufferBuilder.h"
#include "DescriptorBuilder.h"
#include "Texture.h"
#include "PipelineManager.h"

#include <memory>
#include <vector>
#include <string>


namespace CV
{
	class Renderer;
	class ResourceManager
	{
	public:
		explicit ResourceManager(const std::shared_ptr<Renderer>& renderer);
		~ResourceManager();
		vk::Device getDevice() const;
		vk::PhysicalDevice getPhysicalDevice() const;
		[[nodiscard]] vk::DescriptorPool getDescriptorPool() const { return m_descriptorPool; }
		// buffer
		BufferBuilder CreateBufferBuilder();
		// image
		// TODO
		// ImageBuilder CreateImageBuilder();
		
		// descriptor
		void ConfigureDescriptorPoolSizes(const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets);
		DescriptorBuilder CreateDescriptorBuilder();
		vk::DescriptorSet CreateDescriptorSet(vk::DescriptorSetLayout layout);
		vk::DescriptorSet UpdateDescriptorSet(vk::DescriptorSet set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, vk::DeviceSize size, const std::optional<std::vector<CV::Texture>>& textures);

		vk::ShaderModule	getShaderModule(const std::string& shaderPath);
		vk::DescriptorSetLayout getDescriptorSetLayout(const std::string& layoutKey);

		[[nodiscard]] PipelineManager* getPipelineManager() const { return pipelineManager; }


	private:
		PipelineManager* pipelineManager;
		std::shared_ptr<Renderer> _renderer;
		vk::DescriptorPool m_descriptorPool;

		vk::ShaderModule createShaderModule(const std::string& shaderPath);
		vk::DescriptorSetLayout createDescriptorSetLayout(const std::string& layoutKey);

		std::unordered_map<std::string, Texture> m_textureCache;
		std::unordered_map<std::string, vk::DescriptorSetLayout> m_descriptorSetLayoutCache;
		std::unordered_map<std::string, vk::ShaderModule> m_shaderModuleCache;
	};
}

#endif
