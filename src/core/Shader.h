#pragma once
#include "GfxDevice.h"

struct Shader
{
	vk::ShaderModule _resource = nullptr;
	vk::ShaderStageFlagBits _stage = vk::ShaderStageFlagBits::eAll;
	const char* _pEntry = "main";
	std::vector<vk::DescriptorSetLayoutBinding> _layoutBindings{};
	vk::PushConstantRange _pushConstants;
};

typedef std::initializer_list<Shader> Shaders;

struct ShaderDesc
{
	const char* path = "";
	const char* pEntry = "main";
};

Shader CreateShader(GfxDevice& gfxDevice, ShaderDesc desc);
void DestroyShader(GfxDevice& gfxDevice, Shader& shader);
