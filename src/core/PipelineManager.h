#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <string>
#include <unordered_map>

#include "common.h"
#include <vector>

namespace Cravillac
{
	class ResourceManager;

	class PipelineManager
	{
	public:
		PipelineManager(ResourceManager* resourceManager);
		~PipelineManager();

		VkPipeline getPipeline(const std::string& pipelineKey);

		class Builder
		{
		public:
			Builder(PipelineManager* manager);
			Builder& setVertexShader(const std::string& path);
			Builder& setFragmentShader(const std::string& path);
			Builder& setPipelineLayout(VkPipelineLayout pipelineLayout);
			Builder& setVertexInput(
				const  VkVertexInputBindingDescription& binding,
				const std::vector<VkVertexInputAttributeDescription>& attributes
			);
			Builder& setDynamicStates(std::vector<VkDynamicState> dynamicStates);
			Builder& setTopology(VkPrimitiveTopology topology);
			Builder& setDepthTest(bool enable);
			Builder& setBlendMode(const std::string& pipelineKey);
			VkPipeline build(const std::string& pipelineKey);

		private:
			PipelineManager* manager;
			std::string vertShaderPath;
			std::string fragShaderPath;
			std::vector<VkDynamicState> dynamicStates;
			VkPipelineLayout pipelineLayout;
			VkVertexInputBindingDescription vertexBinding;
			std::vector<VkVertexInputAttributeDescription> vertexAttributes;
			VkPrimitiveTopology topology;
			bool depthTest;
			bool blendMode;
		};
		
	private:
		ResourceManager* resourceManager;
		VkDevice device;
		std::unordered_map<std::string, VkPipeline> pipelineCache;
		std::unordered_map<std::string, VkShaderModule> shaderModuleCache;

		VkShaderModule createShaderModule(const std::string& shaderPath);
		VkPipeline createPipeline(const std::string& pipelineKey, const Builder& builder);
	};
}

#endif