#include "Shader.h"

#include <fstream>
#include  <vector>
#include <spirv_reflect.h>

#include "Log.h"

#define SPV_ASSERT(call)	\
	do { \
		SpvReflectResult result = call;\
		assert(result == SPV_REFLECT_RESULT_SUCCESS);\
	} \
	while (0)

static std::vector<char> ReadShaderFile(const std::string & filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        printl(Log::LogLevel::Error, "[SHADER] Failed to open file");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}


static vk::ShaderStageFlagBits GetShaderStage(SpvReflectShaderStageFlagBits reflectShaderStageFlag)
{
	switch (reflectShaderStageFlag)
	{
	case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT :
        return vk::ShaderStageFlagBits::eCompute;
	case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT :
        return vk::ShaderStageFlagBits::eVertex;
	case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return vk::ShaderStageFlagBits::eFragment;

    default:
        assert("Unsupported SpvReflectShaderStageFlagBits!");
        return {};
	}
}

static vk::DescriptorType GetDescriptorType(SpvReflectDescriptorType spvReflectDescriptor)
{
    switch (spvReflectDescriptor)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return vk::DescriptorType::eStorageBuffer;

    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return vk::DescriptorType::eCombinedImageSampler;

    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return vk::DescriptorType::eStorageImage;

    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return vk::DescriptorType::eUniformBuffer;

    default:
        assert(!"Unsupported SpvReflectDescriptorType!");
        return {};
    }
}

Shader CreateShader(GfxDevice& gfxDevice, ShaderDesc desc)
{
    std::vector<char> spvSource = ReadShaderFile(desc.path);

    SpvReflectShaderModule spvModule;
    SPV_ASSERT(spvReflectCreateShaderModule(static_cast<u32>(spvSource.size()), spvSource.data(), &spvModule));

    assert(spvModule.entry_point_count == 1);
    assert(spvModule.descriptor_set_count <= 1);
    assert(spvModule.push_constant_block_count <= 1);

    Shader shader;
    shader._stage = GetShaderStage(spvModule.shader_stage);
    shader._pEntry = desc.pEntry;

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.codeSize = static_cast<u32>(spvSource.size());
    shaderModuleCreateInfo.pCode = reinterpret_cast<u32*>(spvSource.data());

    shader._resource = gfxDevice._device.createShaderModule(shaderModuleCreateInfo, nullptr);

    if (spvModule.push_constant_block_count > 0 && spvModule.push_constant_blocks != nullptr)
    {
        shader._pushConstants.stageFlags = shader._stage;
        shader._pushConstants.offset = spvModule.push_constant_blocks->offset;
        shader._pushConstants.size = spvModule.push_constant_blocks->size;
    }
    u32 spvBindingCount = 0;
    SPV_ASSERT(spvReflectEnumerateDescriptorBindings(&spvModule, &spvBindingCount, nullptr));

    std::vector<SpvReflectDescriptorBinding*> spvReflectDescriptorBindings(spvBindingCount);
    SPV_ASSERT(spvReflectEnumerateDescriptorBindings(&spvModule, &spvBindingCount, spvReflectDescriptorBindings.data()));

    shader._layoutBindings.reserve(spvBindingCount);
    for (u32 layoutBindingIndex = 0; layoutBindingIndex < spvBindingCount; ++layoutBindingIndex)
    {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding{};

        descriptorSetLayoutBinding.binding = spvReflectDescriptorBindings[layoutBindingIndex]->binding;
        descriptorSetLayoutBinding.descriptorCount = MAX_TEXTURES;
        descriptorSetLayoutBinding.descriptorType = GetDescriptorType(spvReflectDescriptorBindings[layoutBindingIndex]->descriptor_type);
        descriptorSetLayoutBinding.stageFlags = shader._stage;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        shader._layoutBindings.push_back(descriptorSetLayoutBinding);
    }

    spvReflectDestroyShaderModule(&spvModule);

    return shader;
}

void DestroyShader(GfxDevice& gfxDevice, Shader& shader)
{
    gfxDevice._device.destroyShaderModule(shader._resource, nullptr);
}

