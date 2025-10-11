#pragma once
#include "GfxDevice.h"
#include "Shader.h"

struct Pipeline
{
	vk::PipelineBindPoint _type;
	vk::DescriptorSetLayout _setLayout = nullptr;
	vk::PipelineLayout _pipelineLayout = nullptr;
	vk::Pipeline _pipeline = nullptr;
	vk::PushConstantRange _pushConstantRange{};
};

struct ColorAttachmentDesc
{
	vk::Format _format = vk::Format::eUndefined;
	bool _bBlendEnable = false;	
};

struct AttachmentLayout
{
	std::initializer_list<ColorAttachmentDesc> _colorAttachments;
	vk::Format _depthStencilFormat = vk::Format::eUndefined;
};

struct RasterizationDesc
{
	vk::CullModeFlags _cullMode = vk::CullModeFlagBits::eBack;
	vk::FrontFace _frontFace = vk::FrontFace::eCounterClockwise;
};

struct DepthStencilDesc
{
	bool _bDepthTestEnable = false;
	bool _bDepthWriteEnable = false;
	vk::CompareOp _depthCompareOp = vk::CompareOp::eGreater;
};

struct GraphicsPipelineDesc
{
	Shaders _shaders;
	AttachmentLayout _attachmentLayout{};
	RasterizationDesc _rasterizationDesc{};
	DepthStencilDesc _depthStencilDesc{};
};

Pipeline CreateGraphicsPipeline(GfxDevice& gfxDevice, GraphicsPipelineDesc desc);
Pipeline CreateComputePipeline(GfxDevice& gfxDevice, Shader& shader);

void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);