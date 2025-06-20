#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Cravillac
{
	class ResourceManager;
	class Renderer;

	class PipelineManager
	{
	public:
		PipelineManager(ResourceManager* resourceManager, const std::shared_ptr<Renderer>& renderer);
		~PipelineManager();	// TODO

		vk::Pipeline getPipeline(const std::string& pipelineKey);
		vk::PipelineLayout getPipelineLayout(const std::string& pipelineLayoutKey);

		class Builder
		{
		public:
			explicit Builder(PipelineManager* manager);
			Builder& setVertexShader(const std::string& path);
			Builder& setMeshShader(const std::string& path);
			Builder& setFragmentShader(const std::string& path);
			Builder& setPipelineLayout(vk::PipelineLayout pipelineLayout);
			Builder& addDescriptorSetLayout(const std::string& key);
			Builder& setDynamicStates(const std::vector<vk::DynamicState>& dynamicStates);
			Builder& setTopology(vk::PrimitiveTopology topology);
			Builder& setDepthTest(bool enable);
			Builder& setBlendMode(bool enable);
			vk::Pipeline build(const std::string& pipelineKey);

			friend class PipelineManager;

		private:
			PipelineManager* m_manager;
			std::string m_vertShaderPath;
			std::string m_meshShaderPath;
			std::string m_fragShaderPath;
			std::vector<vk::DynamicState> m_dynamicStates;
			std::vector<std::string> m_descriptorSetLayoutKeys;
			vk::PipelineLayout m_pipelineLayout;
			vk::PrimitiveTopology m_topology;
			bool m_depthTest;
			bool m_blendMode;
		};

	private:
		ResourceManager* m_resourceManager;
		std::shared_ptr<Renderer> m_renderer;
		std::unordered_map<std::string, vk::Pipeline> m_pipelineCache;
		std::unordered_map<std::string, vk::PipelineLayout> m_pipelineLayoutCache;

		vk::Pipeline createPipeline(const std::string& pipelineKey, const Builder& builder);
		vk::PipelineLayout createPipelineLayout(const std::vector<std::string>& descLayoutKeys);
	};
}

#endif