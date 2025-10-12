#pragma once

#include "GfxDevice.h"
#include "Buffer.h"
struct Sampler
{
	vk::Sampler _resource;
};

struct Texture
{
	u32 _width = 0;
	u32 _height = 0;
	u32 _mipCount = 1;
	u32 _mipIndex = 0;
	vk::Format _format = vk::Format::eUndefined;
	Sampler _sampler{};
	vk::ImageView _imageView = nullptr;
	vk::Image _resource = nullptr;
	VmaAllocation _allocation = nullptr;
	bool _bFromSwapchain = false;
};

struct SamplerDesc
{
	vk::Filter _filterMode = vk::Filter::eLinear;
	vk::SamplerReductionMode _reductionMode = vk::SamplerReductionMode::eWeightedAverage;
	vk::SamplerAddressMode _samplerAddressMode = vk::SamplerAddressMode::eMirroredRepeat;
	vk::SamplerMipmapMode _samplerMipmapMode = vk::SamplerMipmapMode::eNearest;
};

struct TextureViewDesc
{
	u32 _mipIndex{ 0 };
	u32 _mipCount{ 1 };
	SamplerDesc sampler;
};

struct TextureDesc
{
	u32 _width = 0;												
	u32 _height = 0;											
	u32 _mipCount = 1;											
	vk::Format _format = vk::Format::eUndefined;				
	vk::ImageUsageFlags _usage = vk::ImageUsageFlags(0);		
	vk::ImageLayout _layout = vk::ImageLayout::eUndefined;		// Layout for the copy op (with immediate submit)
	vk::AccessFlags _access = vk::AccessFlags(0);				// Access flag for texture image copy op (with immediate submit)
	SamplerDesc _sampler;
	vk::Image _resource = nullptr;								// [Optional] Usually used for swapchain images
	vk::Buffer _copyBuffer = nullptr;							// [Optional] For copying vkBuffer (with texture data) to the Texture
};

Texture CreateTexture(GfxDevice& gfxDevice, TextureDesc desc);
void DestroyTexture(GfxDevice& gfxDevice, Texture& texture);

Texture CreateTextureView(GfxDevice& gfxDevice, TextureViewDesc desc);
void DestroyTextureView(GfxDevice& gfxDevice, Texture& texture);

void TextureBarrier(vk::CommandBuffer& commandBuffer, const Texture& texture, vk::ImageLayout oldLayout,
                    vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
                    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands);

void CopyBufferToImage(vk::CommandBuffer& commandBuffer, vk::Image& texImage,
	vk::Buffer& buffer, u32 width, u32 height);
