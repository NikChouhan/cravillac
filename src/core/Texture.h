#pragma once

#include "Log.h" 
#include "common.hpp"

namespace VKTest
{
    class Renderer;
}

namespace VKTest
{
    class Texture
    {
    public:
        Texture();
        ~Texture();
        void LoadTexture(std::shared_ptr<VKTest::Renderer> renderer ,const char* path);
        //wrl::ComPtr<ID3D11ShaderResourceView> GetTextureView();

    public:
        VkImage m_texImage;
        VkDeviceMemory m_texImageMemory;
    };
}