#include "Texture.h"

static Sampler CreateSampler(vk::Device device, SamplerDesc desc)
{
	Sampler sampler;
	vk::SamplerReductionModeCreateInfo reductionModeCreateInfo;
	reductionModeCreateInfo.reductionMode = desc._reductionMode;

	vk::SamplerCreateInfo samplerCreateInfo;

	samplerCreateInfo.pNext = &reductionModeCreateInfo;
	samplerCreateInfo.magFilter = desc._filterMode;
	samplerCreateInfo.minFilter = desc._filterMode;
	samplerCreateInfo.mipmapMode = desc._samplerMipmapMode;
	samplerCreateInfo.addressModeU = desc._samplerAddressMode;
	samplerCreateInfo.addressModeV = desc._samplerAddressMode;
	samplerCreateInfo.addressModeW = desc._samplerAddressMode;
	samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerCreateInfo.maxAnisotropy = 8.f;
	samplerCreateInfo.anisotropyEnable = vk::False;

	sampler._resource = device.createSampler(samplerCreateInfo, nullptr);
	return sampler;
}

static vk::ImageAspectFlags GetAspectMask(vk::Format format)
{
	return format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
}

static vk::ImageView CreateImageView(vk::Device device, vk::Image image, vk::Format format, u32 baseMip, u32 mipCount)
{
	vk::ImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.subresourceRange.aspectMask = GetAspectMask(format);
	imageViewCreateInfo.subresourceRange.baseMipLevel = baseMip;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = device.createImageView(imageViewCreateInfo, nullptr);

	return imageView;
}

Texture CreateTexture(GfxDevice& gfxDevice, TextureDesc desc)
{
	Texture texture;
	texture._width = desc._width;
	texture._height = desc._height;
	texture._mipCount = desc._mipCount;
	texture._resource = desc._resource;
	texture._format = desc._format;
	texture._bFromSwapchain = desc._resource != nullptr;

	if (texture._resource == nullptr)
	{
		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.extent.width = desc._width;
		imageCreateInfo.extent.height = desc._height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = desc._mipCount;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
		imageCreateInfo.usage = desc._usage;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageCreateInfo.format = texture._format;

		vma::AllocationCreateInfo vmaAllocationCreateInfo;
		vmaAllocationCreateInfo.usage = vma::MemoryUsage::eGpuOnly;
		vmaAllocationCreateInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

		VK_ASSERT(gfxDevice._allocator.createImage(&imageCreateInfo, 
			&vmaAllocationCreateInfo, &texture._resource, &texture._allocation, nullptr));
	}

	texture._sampler = CreateSampler(gfxDevice._device, desc._sampler);
	texture._imageView = CreateImageView(gfxDevice._device, texture._resource, texture._format, 0, desc._mipCount);

	if (desc._layout != vk::ImageLayout::eUndefined && desc._copyBuffer)
	{
		ImmediateSubmit(gfxDevice, [&](vk::CommandBuffer commandBuffer)
			{
				TextureBarrier(commandBuffer, texture, vk::ImageLayout::eUndefined,
					vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eNone, desc._access);
			});
		ImmediateSubmit(gfxDevice, [&](vk::CommandBuffer commandBuffer)
			{
				CopyBufferToImage(commandBuffer, texture._resource, desc._copyBuffer, desc._width, desc._height);
			});
		ImmediateSubmit(gfxDevice, [&](vk::CommandBuffer commandBuffer)
			{
				TextureBarrier(commandBuffer, texture, vk::ImageLayout::eTransferDstOptimal,
					desc._layout, vk::AccessFlagBits::eNone, desc._access);
			});
	}


	return texture;
}

static void DestroySampler(const GfxDevice& gfxDevice, Sampler sampler)
{
	gfxDevice._device.destroySampler(sampler._resource, nullptr);
}

void DestroyTexture(GfxDevice& gfxDevice, Texture& texture)
{
	gfxDevice._device.destroyImageView(texture._imageView, nullptr);
	DestroySampler(gfxDevice, texture._sampler);

	if (texture._bFromSwapchain)
	{
		vmaDestroyImage(gfxDevice._allocator, texture._resource, texture._allocation);
	}
}

void DestroyTextureView(GfxDevice& gfxDevice, Texture& texture)
{
	gfxDevice._device.destroyImageView(texture._imageView, nullptr);
	DestroySampler(gfxDevice, texture._sampler);
}

// used for transition of images mostly
void TextureBarrier(vk::CommandBuffer& commandBuffer, const Texture& texture, vk::ImageLayout oldLayout,
                    vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                    vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask)
{
	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = GetAspectMask(texture._format);
	subresourceRange.baseMipLevel = texture._mipIndex;
	subresourceRange.levelCount = texture._mipCount;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	vk::ImageMemoryBarrier imageMemoryBarrier;

	imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.image = texture._resource;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, static_cast<vk::DependencyFlags>(0), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

void CopyBufferToImage(vk::CommandBuffer& commandBuffer, vk::Image& texImage, vk::Buffer& buffer, u32 width, u32 height)
{

	vk::BufferImageCopy2 region{};
	region.sType = vk::StructureType::eBufferImageCopy2;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = vk::Extent3D{ width, height, 1 };

	vk::CopyBufferToImageInfo2 bufferCI{};
	bufferCI.sType = vk::StructureType::eCopyBufferToImageInfo2;
	bufferCI.srcBuffer = buffer;
	bufferCI.regionCount = 1;
	bufferCI.pRegions = &region;
	bufferCI.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
	bufferCI.dstImage = texImage;

	commandBuffer.copyBufferToImage2(&bufferCI);
}
