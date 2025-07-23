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

namespace CV
{
    void Texture::LoadTexture(const std::shared_ptr<Renderer>& renderer, const char *filename)
    {
        _renderer = renderer;
        // Load the image using stb_image
        int width, height, channels;
        //stbi_set_flip_vertically_on_load(true); // Flip the image vertically for DirectX
        unsigned char *imgData = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

        if (!imgData)
        {
            printl(Log::LogLevel::Error,"[STB] Failed to load texture");
            return;
        }
        else
        {
            vk::DeviceSize imageSize = width * height * 4;

            vk::Buffer stagingBuffer{};
            vk::DeviceMemory stagingBufferMemory{};

            CreateBuffer(renderer->_device, renderer->_physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         stagingBuffer, stagingBufferMemory);

            void *data;
            vkMapMemory(renderer->_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, imgData, static_cast<size_t>(imageSize));
            vkUnmapMemory(renderer->_device, stagingBufferMemory);
            stbi_image_free(imgData);

            // allocate memory inside device (gpu) to upload the texture, bind it to the image memory handle
            // (watch Tu Wien lecture for more info. TLDR; Vulkan can allocate the memory anywhere inside the hw optimally,
            // and the image memory handle is whats used to access it)
            CreateImage(renderer->_physicalDevice, renderer->_device, width, height, vk::Format::eR8G8B8A8Srgb,
                        vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                        vk::MemoryPropertyFlagBits::eDeviceLocal, m_texImage, m_texImageMemory);

            /* remember: The actual uploading of texture from storage to VRAM occurs here.
             * The command buffers do the work related to it. So its important to target
             * this place when I implement a proper texture streaming
			*/
            vk::CommandBuffer tempCmdBuffer = BeginSingleTimeCommands(renderer->_device, renderer->_commandPool);

            TransitionImage(tempCmdBuffer, m_texImage, {}, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(tempCmdBuffer, m_texImage, stagingBuffer, width, height);
            TransitionImage(tempCmdBuffer, m_texImage, vk::ImageLayout::eTransferDstOptimal,
                            vk::ImageLayout::eShaderReadOnlyOptimal);

            EndSingleTimeCommands(renderer->_device, renderer->_graphicsQueue, renderer->_commandPool, tempCmdBuffer);

            vkDestroyBuffer(renderer->_device, stagingBuffer, nullptr);
            vkFreeMemory(renderer->_device, stagingBufferMemory, nullptr);

            CreateTextureImageView();
            CreateTextureSampler();
        }
    }
    void Texture::CreateTextureImageView()
    {
        m_texImageView = CreateImageView(_renderer->_device, m_texImage, vk::Format::eR8G8B8A8Srgb,
                                         vk::ImageAspectFlagBits::eColor);
    }
    void Texture::CreateTextureSampler()
    {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;

        vk::PhysicalDeviceProperties2 properties{};
        properties = _renderer->_physicalDevice.getProperties2();

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
            m_texSampler = _renderer->_device.createSampler(samplerInfo);
            //printl(Log::LogLevel::Info,"[TEXTURE] Success to create Texture Sampler");
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[TEXTURE] Failure to create Texture Sampler: {}", std::string(err.what()));
            m_texSampler = nullptr;
        }
    }
};