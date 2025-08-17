#pragma once
#include "GfxDevice.h"

struct Shader
{
	vk::ShaderModule resource = nullptr;
	vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eAll;
	const char* pEntry = "main";
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{};
	vk::PushConstantRange pushConstants;
};

typedef std::initializer_list<Shader> Shaders;

struct ShaderDesc
{
	const char* path = "";
	const char* pEntry = "main";
};

Shader CreateShader(GfxDevice& gfxDevice, ShaderDesc desc);
void DestroyShader(GfxDevice& gfxDevice, Shader& shader);
