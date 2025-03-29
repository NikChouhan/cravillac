#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "common.h"
#include "BufferBuilder.h"
#include "ImageBuilder.h"
#include "DescriptorBuilder.h"
#include "Texture.h"
#include "PipelineManager.h"

#include <memory>


namespace Cravillac
{
	class Renderer;
	class ResourceManager
	{
	public:
		ResourceManager(std::shared_ptr<Renderer> renderer);
		~ResourceManager();
		VkDevice getDevice();
		VkPhysicalDevice getPhysicalDevice();
		VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		// buffer
		BufferBuilder CreateBufferBuilder();
		// image
		// TODO
		// ImageBuilder CreateImageBuilder();
		
		// descriptor
		void ConfigureDescriptorPoolSizes(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
		DescriptorBuilder CreateDescriptorBuilder();
		VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout layout);
		VkDescriptorSet UpdateDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkDeviceSize size, std::vector<Cravillac::Texture>* textures);

		VkShaderModule	getShaderModule(const std::string& shaderPath);
		VkDescriptorSetLayout getDescriptorSetLayout(const std::string& layoutKey);

		PipelineManager* getPipelineManager() { return pipelineManager; }


	private:
		PipelineManager* pipelineManager;
		std::shared_ptr<Renderer> m_renderer;
		VkDescriptorPool m_descriptorPool;

		VkShaderModule createShaderModule(const std::string& shaderPath);
		VkDescriptorSetLayout createDescriptorSetLayout(const std::string& layoutKey);

		std::unordered_map<std::string, Texture> m_textureCache;
		std::unordered_map<std::string, VkDescriptorSetLayout> m_descriptorSetLayoutCache;
		std::unordered_map<std::string, VkShaderModule> m_shaderModuleCache;
	};
}

#endif
