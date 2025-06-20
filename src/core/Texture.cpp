#include <pch.h>

#include "Texture.h"
#include "renderer.h"
#include "Log.h"
#include <vk_utils.h>

//#include <cstddef>
#define STB_IMAGE_IMPLEMENTATION
#include <includes/stb_image.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace Cravillac
{
    void Texture::LoadTexture(const std::shared_ptr<Renderer>& renderer, const char *filename)
    {
        m_renderer = renderer;
        // Load the image using stb_image
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true); // Flip the image vertically for DirectX
        unsigned char *imgData = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

        if (!imgData)
        {
            Log::Error("[STB] Failed to load texture");
            return;
        }
        else
        {
            vk::DeviceSize imageSize = width * height * 4;

            vk::Buffer stagingBuffer{};
            vk::DeviceMemory stagingBufferMemory{};

            CreateBuffer(renderer->m_device, renderer->m_physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

            void *data;
            vkMapMemory(renderer->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, imgData, static_cast<size_t>(imageSize));
            vkUnmapMemory(renderer->m_device, stagingBufferMemory);
            stbi_image_free(imgData);

            CreateImage(renderer->m_physicalDevice, renderer->m_device, width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_texImage, m_texImageMemory);

            vk::CommandBuffer tempCmdBuffer = BeginSingleTimeCommands(renderer->m_device, renderer->m_commandPool);

            TransitionImage(tempCmdBuffer, m_texImage, {}, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(tempCmdBuffer, m_texImage, stagingBuffer, width, height);
            TransitionImage(tempCmdBuffer, m_texImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            EndSingleTimeCommands(renderer->m_device, renderer->m_graphicsQueue, renderer->m_commandPool, tempCmdBuffer);

            vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
            vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

            CreateTextureImageView();
            CreateTextureSampler();
        }
    }
    void Texture::CreateTextureImageView()
    {
        m_texImageView = CreateImageView(m_renderer->m_device, m_texImage, vk::Format::eR8G8B8A8Srgb,vk::ImageAspectFlagBits::eColor);
    }
    void Texture::CreateTextureSampler()
    {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;

        vk::PhysicalDeviceProperties2 properties{};
        properties = m_renderer->m_physicalDevice.getProperties2();

        samplerInfo.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eMirroredRepeat;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        try
        {
            m_texSampler = m_renderer->m_device.createSampler(samplerInfo);
            Log::Info("[TEXTURE] Success to create Texture Sampler");
        }
        catch (vk::SystemError& err)
        {
            Log::Error("[TEXTURE] Failure to create Texture Sampler: " + std::string(err.what()));
            m_texSampler = nullptr;
        }
    }
};