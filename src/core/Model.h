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
    class Texture;
    struct Meshlet;
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
        Position = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        Rotation = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        Scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    }

    DirectX::XMMATRIX Matrix{};
    DirectX::XMVECTOR Position {};
    DirectX::XMVECTOR Rotation{};
    DirectX::XMVECTOR Scale = {};
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

    vk::ImageView AlbedoView = nullptr;
    vk::ImageView NormalView = nullptr;
    vk::ImageView MetallicRoughnessView = nullptr;
    vk::ImageView EmissiveView = nullptr;
    vk::ImageView AOView = nullptr;

    DirectX::XMFLOAT3 FlatColor;
};

struct Mesh
{
    std::vector<Cravillac::Vertex> vertices;
    std::vector<u32> indices;
    u32 vertexCount;
    u32 indexCount;
};

struct MeshInfo
{
    size_t vertexCount = 0;
    size_t indexCount = 0;
    uint32_t materialIndex = 0;
    uint32_t startIndex = 0;
    uint32_t startVertex = 0;
    Transformation transform;
    DirectX::XMMATRIX normalMatrix;
};

namespace Cravillac
{
    class Model
    {
    public:
        Model();
        ~Model();
        void LoadModel(const std::shared_ptr<Renderer>& renderer, const std::string& path);
        DirectX::XMMATRIX ComputeNormalMatrix(const DirectX::XMMATRIX &worldMatrix);
        void SetBuffers();
    private:
        void ProcessNode(cgltf_node *node, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        void ProcessMesh(cgltf_primitive *primitive, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        void OptimiseMesh(MeshInfo& meshInfo, Mesh& mesh);
        void ProcessMeshlets(Mesh& mesh);
        bool LoadMaterialTexture(Material& mat, const cgltf_texture_view* textureView, TextureType type);

        void ValidateResources() const;

        //ConstantBuffer cb;
        MaterialConstants matColor;

    public:
        std::string m_dirPath;
        std::vector<Vertex> m_vertices;
        std::vector<u32> m_indices;
        std::vector<MeshInfo> m_meshes;
        std::vector<Meshlet> m_meshlets;
        std::vector<Material> m_materials;

        std::shared_ptr<Renderer> m_renderer;


        vk::Buffer m_vertexBuffer = VK_NULL_HANDLE;
        vk::Buffer m_indexBuffer = VK_NULL_HANDLE;
        vk::Buffer m_meshletBuffer = VK_NULL_HANDLE;

        std::vector<Texture> modelTextures;
    private:


        vk::DeviceMemory m_vertexMemory = VK_NULL_HANDLE;
        vk::DeviceMemory m_indexMemory = VK_NULL_HANDLE;
        vk::DeviceMemory m_meshletMemory = VK_NULL_HANDLE;

        std::unordered_set<std::string> loadedTextures; // To track loaded textures
        std::unordered_map<cgltf_material*, size_t> materialLookup;

        ResourceManager* m_resourceManager;

    };
}