#include "Texture.h"
#include "renderer.h"
#include "Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace VKTest
{
    Texture::Texture()
    {
    }


    Texture::~Texture()
    {
    }

    void Texture::LoadTexture(std::shared_ptr<VKTest::Renderer> renderer, const char* filename)
    {
        // Load the image using stb_image
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);  // Flip the image vertically for DirectX
        unsigned char* imgData = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

        if (!imgData) {
            Log::Error("[STB] Failed to load texture");
        }
        else
        {
            VkDeviceSize imageSize = width * height * 4;

            VkBuffer stagingBuffer{};
            VkDeviceMemory stagingBufferMemory{};

            void* data;
            vkMapMemory(renderer->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, imgData, static_cast<size_t>(imageSize));
            vkUnmapMemory(renderer->m_device, stagingBufferMemory);
            stbi_image_free(imgData);
        }

        VkImageCreateInfo imageCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .mipLevels = 1,
            .arrayLayers = 1
        };

        imageCI.extent.width = static_cast<uint32_t>(width);
        imageCI.extent.height = static_cast<uint32_t>(height);
        imageCI.extent.depth = 1;

        imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.flags = NULL;

        if (vkCreateImage(renderer->m_device, &imageCI, nullptr, &m_texImage) != VK_SUCCESS)
        {
            Log::Error("[STB] Failed to Create Texture Image");
        }
        else Log::Info("[STB] Success to Create Texture Image");

        VkMemoryRequirements mmRequirements{};
        vkGetImageMemoryRequirements(renderer->m_device, m_texImage, &mmRequirements);

        VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mmRequirements.size,
            //.memoryTypeIndex = renderer
        };
    }
};