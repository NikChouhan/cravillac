#include <algorithm>
#include <vector>
#include "Pipeline.h"

#include "Vertex.h"

static vk::DescriptorSetLayout CreateDescriptorSetLayout(vk::Device device, const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings)
{
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = layoutBindings.size();
	setLayoutCreateInfo.pBindings = layoutBindings.data();
	setLayoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

	vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(setLayoutCreateInfo, nullptr);
	return descriptorSetLayout;
}

static vk::PipelineLayout CreatePipelineLayout(vk::Device device, vk::DescriptorSetLayout setLayout, vk::PushConstantRange pushConstant)
{
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);
	return pipelineLayout;
}

static bool MergeBindingShaderStage(std::vector<vk::DescriptorSetLayoutBinding>& mergeLayoutBIndings, vk::DescriptorSetLayoutBinding layoutBinding)
{
	for (vk::DescriptorSetLayoutBinding& shaderBinding : mergeLayoutBIndings)
	{
		if (shaderBinding.binding == layoutBinding.binding)
		{
			shaderBinding.stageFlags |= layoutBinding.stageFlags;
			return true;
		}
	}
	return false;
}

static std::vector<vk::DescriptorSetLayoutBinding> MergeSetLayoutBindings(const Shaders& shaders)
{
	std::vector<vk::DescriptorSetLayoutBinding> mergeLayoutBIndings;

	for (const Shader& shader : shaders)
	{
		for (const vk::DescriptorSetLayoutBinding& layoutBinding : shader._layoutBindings)
		{
			if (MergeBindingShaderStage(mergeLayoutBIndings, layoutBinding))
			{
				continue;
			}
			mergeLayoutBIndings.push_back(layoutBinding);
		}
	}

	std::ranges::sort(mergeLayoutBIndings);

	return mergeLayoutBIndings;
}

static vk::PushConstantRange MergePushConstants(Shaders& shaders)
{
	vk::PushConstantRange mergePushConstants{};

	for (const Shader& shader : shaders)
	{
		if (shader._pushConstants.stageFlags)
		{
			if (!mergePushConstants.stageFlags)
			{
				mergePushConstants = {
					.offset = shader._pushConstants.offset,
					.size = shader._pushConstants.size };
			}

			assert(mergePushConstants.offset == shader._pushConstants.offset);
			assert(mergePushConstants.size == shader._pushConstants.size);

			mergePushConstants.stageFlags |= shader._pushConstants.stageFlags;
		}
	}

	return mergePushConstants;
}

static Pipeline CreatePipeline(GfxDevice& gfxDevice, vk::PipelineBindPoint pipelineBindPoint, Shaders shaders, LAMBDA(Pipeline&) callback)
{
	Pipeline pipeline;
	pipeline._type = pipelineBindPoint;

	//vk::PushConstantRange pushConstant = MergePushConstants(shaders);
	vk::PushConstantRange pushConstant;
	pushConstant.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(PushConstants);

	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = MergeSetLayoutBindings(shaders);

	pipeline._setLayout = CreateDescriptorSetLayout(gfxDevice._device, layoutBindings);

	pipeline._pipelineLayout = CreatePipelineLayout(gfxDevice._device, pipeline._setLayout, pushConstant);



	callback(pipeline);

	return pipeline;
}

static vk::Pipeline CreateGraphicsPipeline(vk::Device device, vk::PipelineLayout& pipelineLayout, GraphicsPipelineDesc desc)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	for (const auto& shader : desc._shaders)
	{
		vk::PipelineShaderStageCreateInfo shaderStageCreateInfo;
		shaderStageCreateInfo.stage = shader._stage;
		shaderStageCreateInfo.module = shader._resource;
		shaderStageCreateInfo.pName = shader._pEntry;
		shaderStages.push_back(shaderStageCreateInfo);
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

	vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
	dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	vk::PipelineRasterizationStateCreateInfo pipelineRasterizationCI;
	pipelineRasterizationCI.depthClampEnable = VK_FALSE;
	pipelineRasterizationCI.rasterizerDiscardEnable = VK_FALSE;
	pipelineRasterizationCI.polygonMode = vk::PolygonMode::eFill;
	pipelineRasterizationCI.cullMode = desc._rasterizationDesc._cullMode;
	pipelineRasterizationCI.frontFace = desc._rasterizationDesc._frontFace;
	pipelineRasterizationCI.depthBiasEnable = VK_FALSE;
	pipelineRasterizationCI.depthBiasConstantFactor = 0.0f;
	pipelineRasterizationCI.depthBiasClamp = 0.0f;
	pipelineRasterizationCI.depthBiasSlopeFactor = 0.0f;
	pipelineRasterizationCI.lineWidth = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
	colorBlendAttachments.reserve(desc._attachmentLayout._colorAttachments.size());

	for (auto colorAttachmentState : desc._attachmentLayout._colorAttachments)
	{
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.blendEnable = colorAttachmentState._bBlendEnable;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachments.push_back(colorBlendAttachment);
	}

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants = { { 0.0f, 0.0f, 0.0f, 0.0f } };

	std::vector<vk::Format> colorFormats;
	colorFormats.reserve(desc._attachmentLayout._colorAttachments.size());

	for (ColorAttachmentDesc colorAttachmentState : desc._attachmentLayout._colorAttachments)
	{
		colorFormats.push_back(colorAttachmentState._format);
	}

	vk::PipelineDepthStencilStateCreateInfo depthInfo;
	depthInfo.depthTestEnable = desc._depthStencilDesc._bDepthTestEnable;
	depthInfo.depthWriteEnable = desc._depthStencilDesc._bDepthWriteEnable;
	depthInfo.depthCompareOp = desc._depthStencilDesc._depthCompareOp;
	depthInfo.depthBoundsTestEnable = VK_FALSE;
	depthInfo.stencilTestEnable = VK_FALSE;
	depthInfo.front = vk::StencilOpState{};
	depthInfo.back = vk::StencilOpState{};
	depthInfo.minDepthBounds = 0.0f;
	depthInfo.maxDepthBounds = 1.0f;

	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = u32(colorFormats.size());
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = desc._attachmentLayout._depthStencilFormat;

	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;

	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
	graphicsPipelineCreateInfo.stageCount = shaderStages.size();
	graphicsPipelineCreateInfo.pStages = shaderStages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationCI;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = nullptr;
	graphicsPipelineCreateInfo.basePipelineHandle = nullptr;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	vk::Pipeline graphicsPipeline;
	VK_ASSERT(device.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline));
	// OR
	//auto result = device.createGraphicsPipelines(nullptr, { graphicsPipelineCreateInfo });
	//vk::Pipeline graphicsPipeline = result.value[0];
	return graphicsPipeline;
}

static vk::Pipeline CreateComputePipeline(vk::Device device, vk::PipelineLayout, const Shader& shader)
{
	vk::Pipeline computePipeline;

	return computePipeline;
}

Pipeline CreateGraphicsPipeline(GfxDevice& gfxDevice, GraphicsPipelineDesc desc)
{
	return CreatePipeline(gfxDevice, vk::PipelineBindPoint::eGraphics, desc._shaders,
		[&](Pipeline& pipeline)
		{
			pipeline._pipeline = CreateGraphicsPipeline(gfxDevice._device, pipeline._pipelineLayout, desc);
		});
}
// merge push constant is for combining the shader stages so that the push constant is valid across all shaders per pipeline. Ex a push constant can be accessed by
// a mesh shader, frag shader both with this

Pipeline CreateComputePipeline(GfxDevice& gfxDevice, Shader& shader)
{
	return CreatePipeline(gfxDevice, vk::PipelineBindPoint::eCompute, { shader },
		[&](Pipeline& pipeline)
		{
			pipeline._pipeline = CreateComputePipeline(gfxDevice._device, pipeline._pipelineLayout, shader);
		});
}

void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline)
{
	gfxDevice._device.destroyPipeline(pipeline._pipeline, nullptr);
	gfxDevice._device.destroyPipelineLayout(pipeline._pipelineLayout, nullptr);
	gfxDevice._device.destroyDescriptorSetLayout(pipeline._setLayout, nullptr);
}
