#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <vulkan/vulkan.hpp>

namespace CV
{
    class Renderer;
}

namespace CV
{
    class Texture
    {
    public:
        Texture() = default;
        void LoadTexture(const std::shared_ptr<Renderer>& renderer, const char *filename);
        void CreateTextureImageView();
        void CreateTextureSampler();

    public:
        vk::Image m_texImage = VK_NULL_HANDLE;
        vk::DeviceMemory m_texImageMemory = VK_NULL_HANDLE;
        vk::ImageView m_texImageView = VK_NULL_HANDLE;
        vk::Sampler m_texSampler = VK_NULL_HANDLE;
        std::shared_ptr<Renderer> _renderer;
    };
}

#endif