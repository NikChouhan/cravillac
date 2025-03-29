#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

#include <memory>

namespace Cravillac
{
    class Renderer;
}

namespace Cravillac
{
    class Texture
    {
    public:
        Texture() {}
        void LoadTexture(std::shared_ptr<Renderer> renderer, const char* path);
        void CreateTextureImageView();
        void CreateTextureSampler();

    public:
        VkImage m_texImage = VK_NULL_HANDLE;
        VkDeviceMemory m_texImageMemory = VK_NULL_HANDLE;
        VkImageView m_texImageView = VK_NULL_HANDLE;
        VkSampler m_texSampler = VK_NULL_HANDLE;
        std::shared_ptr<Cravillac::Renderer> renderer;
    };
}

#endif