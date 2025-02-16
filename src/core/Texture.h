#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

#include <memory>

namespace VKTest
{
    class Renderer;
}

namespace VKTest
{
    class Texture
    {
    public:
        Texture(std::shared_ptr<Renderer> renderer);
        ~Texture();
        void LoadTexture(const char* path);
        void CreateTextureImageView();
        void CreateTextureSampler();
    public:
        VkImage m_texImage;
        VkDeviceMemory m_texImageMemory;
        VkImageView m_texImageView;
        VkSampler m_texSampler;
    private:
        std::shared_ptr<VKTest::Renderer> renderer;
    };
}

#endif