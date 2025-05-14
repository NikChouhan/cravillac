#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <cgltf.h>
#include <DirectXMath.h>
#include <memory>
#include "StandardTypes.h"
#include "common.h"
#include "ResourceManager.h"

namespace Cravillac
{
    class Renderer;
    struct Vertex;
}

enum class TextureType
{
    ALBEDO,
    NORMAL,
    METALLIC_ROUGHNESS,
    EMISSIVE,
    AO,
    SPECULAR,
    DISPLACEMENT,
    OPACITY,
    GLOSSINESS,
    HEIGHT,
    CUBEMAP,
    BRDF_LUT,
    SPECULAR_GLOSSINESS
};

struct Transformation
{
    Transformation()
    {
        Matrix = DirectX::XMMatrixIdentity();
        DirectX::XMVECTOR Position = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        DirectX::XMVECTOR Rotation = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        DirectX::XMVECTOR Scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    }

    DirectX::XMMATRIX Matrix = DirectX::XMMatrixIdentity();
    DirectX::XMVECTOR Position = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    DirectX::XMVECTOR Rotation = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    DirectX::XMVECTOR Scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
};


struct MaterialConstants
{
    DirectX::XMFLOAT4 ambientColor;
    DirectX::XMFLOAT4 diffuseColor;
    DirectX::XMFLOAT4 specularColor;
    float specularPower;
};

struct Material
{
    bool HasAlbedo = false;
    bool HasNormal = false;
    bool HasMetallicRoughness = false;
    bool HasEmissive = false;
    bool HasAO = false;

    std::string AlbedoPath;
    std::string NormalPath;
    std::string MetallicRoughnessPath;
    std::string EmissivePath;
    std::string AOPath;

    VkImageView AlbedoView = nullptr;
    VkImageView NormalView = nullptr;
    VkImageView MetallicRoughnessView = nullptr;
    VkImageView EmissiveView = nullptr;
    VkImageView AOView = nullptr;

    DirectX::XMFLOAT3 FlatColor;
};

struct Primitive
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
    uint32_t startIndex = 0;
    uint32_t startVertex = 0;
    Transformation transform;
};

namespace Cravillac
{
    class Model
    {
    public:
        Model();
        ~Model();
        void LoadModel(const std::shared_ptr<Renderer>& renderer, std::string path);
        void SetBuffers();
        bool SetTexResources(uint32_t materialIndex);
        void UpdateCB(Primitive prim);

        void Render();

    private:
        void ProcessNode(cgltf_node *node, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        void ProcessPrimitive(cgltf_primitive *primitive, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        HRESULT LoadMaterialTexture(Material& mat, const cgltf_texture_view* textureView, TextureType type) const;

        void ValidateResources() const;

        //ConstantBuffer cb;
        MaterialConstants matColor;

    public:
        u32 vertexOffset = 0;
        u32 indexOffset = 0;
        //wrl::ComPtr<ID3D11ShaderResourceView> m_textureView = nullptr;
        std::string m_dirPath;
        std::string name;
        std::vector<Vertex> m_vertices;
        std::vector<u32> m_indices;
        std::vector<Primitive> m_primitives;
        std::vector<Material> m_materials;

        std::shared_ptr<Renderer> renderer;


        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    private:


        VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
        VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;

        //wrl::ComPtr<ID3D11Buffer> m_vertexBuffer;
        //wrl::ComPtr<ID3D11Buffer> m_indexBuffer;
        //wrl::ComPtr<ID3D11Buffer> m_constantBuffer;
        //UINT m_vertexCount;
        //UINT m_indexCount;

        //wrl::ComPtr<ID3D11Buffer> m_materialBuffer;
        //wrl::ComPtr<ID3D11SamplerState> m_samplerState = nullptr;

        std::unordered_set<std::string> loadedTextures; // To track loaded textures
        std::unordered_map<cgltf_material*, int> materialLookup;

        ResourceManager* m_resourceManager;

    };
}