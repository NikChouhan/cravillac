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
        this->renderer = renderer;
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
            VkDeviceSize imageSize = width * height * 4;

            VkBuffer stagingBuffer{};
            VkDeviceMemory stagingBufferMemory{};

            CreateBuffer(renderer->m_device, renderer->m_physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void *data;
            vkMapMemory(renderer->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, imgData, static_cast<size_t>(imageSize));
            vkUnmapMemory(renderer->m_device, stagingBufferMemory);
            stbi_image_free(imgData);

            CreateImage(renderer->m_physicalDevice, renderer->m_device, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texImage, m_texImageMemory);

            VkCommandBuffer tempCmdBuffer = BeginSingleTimeCommands(renderer->m_device, renderer->m_commandPool);

            TransitionImage(tempCmdBuffer, m_texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            CopyBufferToImage(tempCmdBuffer, m_texImage, stagingBuffer, width, height);
            TransitionImage(tempCmdBuffer, m_texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            EndSingleTimeCommands(renderer->m_device, renderer->m_graphicsQueue, renderer->m_commandPool, tempCmdBuffer);

            vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
            vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

            CreateTextureImageView();
            CreateTextureSampler();
        }
    }
    void Texture::CreateTextureImageView()
    {
        m_texImageView = CreateImageView(renderer->m_device, m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    void Texture::CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
        };

        VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        vkGetPhysicalDeviceProperties2(renderer->m_physicalDevice, &properties);

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(renderer->m_device, &samplerInfo, nullptr, &m_texSampler) != VK_SUCCESS)
        {
            Log::Error("[TEXTURE] Failure to create Texture Sampler");
            m_texSampler = VK_NULL_HANDLE;
        }
        else
            Log::Info("[TEXTURE] Success to create Texture Sampler");
    }
};