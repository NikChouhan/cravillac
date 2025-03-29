#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <string>
#include <unordered_map>
#include <array>
#include <memory>

#include "common.h"
#include <vector>


namespace Cravillac
{
	class ResourceManager;
	class Renderer;

	class PipelineManager
	{
	public:
		PipelineManager(ResourceManager* resourceManager, std::shared_ptr<Renderer> renderer);
		~PipelineManager();

		VkPipeline getPipeline(const std::string& pipelineKey);
		VkPipelineLayout getPipelineLayout(const std::vector<std::string>& descLayoutKeys);


		class Builder
		{
		public:
			Builder(PipelineManager* manager);
			Builder& setVertexShader(const std::string& path);
			Builder& setFragmentShader(const std::string& path);
			Builder& setPipelineLayout(VkPipelineLayout pipelineLayout);
			Builder& addDescriptorSetLayout(const std::string& key);
			Builder& setVertexInput(
				const  VkVertexInputBindingDescription& binding,
				const std::array<VkVertexInputAttributeDescription, 3>& attributes
			);
			Builder& setDynamicStates(std::vector<VkDynamicState> dynamicStates);
			Builder& setTopology(VkPrimitiveTopology topology);
			Builder& setDepthTest(bool enable);
			Builder& setBlendMode(bool enable);
			VkPipeline build(const std::string& pipelineKey);
			friend class PipelineManager;

		private:
			PipelineManager* m_manager;
			std::string m_vertShaderPath;
			std::string m_fragShaderPath;
			std::vector<VkDynamicState> m_dynamicStates;
			std::vector<std::string> m_descriptorSetLayoutKeys;
			VkPipelineLayout m_pipelineLayout;
			VkVertexInputBindingDescription m_vertexBinding;
			std::array<VkVertexInputAttributeDescription, 3> m_vertexAttributes;
			VkPrimitiveTopology m_topology;
			bool m_depthTest;
			bool m_blendMode;
		};
		
	private:
		ResourceManager* m_resourceManager;
		std::shared_ptr<Renderer> m_renderer;
		std::unordered_map<std::string, VkPipeline> m_pipelineCache;
		std::unordered_map<std::string, VkPipelineLayout> m_pipelineLayoutCache;

		VkPipeline createPipeline(const std::string& pipelineKey, const Builder& builder);
		VkPipelineLayout createPipelineLayout(const std::vector<std::string>& descLayoutKeys);
	};
}

#endif