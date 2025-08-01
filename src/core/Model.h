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

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace CV
{
    class Renderer;
    struct Vertex;
    class Texture;
    struct Meshlet;
}

enum class TextureType
{
    ALBEDO = 1,
    NORMAL = 2,
    METALLIC_ROUGHNESS = 4,
    EMISSIVE = 8,
    SPECULAR = 16
};

struct Transformation
{
    Transformation()
    {
        Matrix = glm::mat4(1.f);
        Position = {};
        Rotation = {};
        Scale = {};
    }

    glm::mat4 Matrix{};
    glm::vec3 Position{};
    glm::vec3 Rotation{};
    glm::vec3 Scale = {};
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

    u32 albedoIndex = -1;
    u32 normalIndex = -1;
    u32 emmisiveIndex = -1;
    u32 metallicIndex = -1;

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
    std::vector<CV::Vertex> vertices;
    std::vector<u32> indices;
    u32 vertexCount;
    u32 indexCount;
};


struct MeshInfo
{
    size_t vertexCount = 0;
    size_t indexCount = 0;
    u32 materialIndex = -1;
    uint32_t startIndex = 0;
    uint32_t startVertex = 0;
    Transformation transform;
    glm::mat4 normalMatrix;
};

namespace CV
{
    class Model
    {
    public:
        Model();
        ~Model();
        void LoadModel(const std::shared_ptr<Renderer>& renderer, const std::string& path);
        glm::mat4 ComputeNormalMatrix(const glm::mat4 worldMatrix);
        void SetBuffers();
    private:
        void ProcessNode(cgltf_node *node, const cgltf_data *data, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        void ProcessMesh(cgltf_primitive *primitive, std::vector<Vertex> &vertices, std::vector<u32> &indices, Transformation& parentTransform);
        void OptimiseMesh(MeshInfo& meshInfo, Mesh& mesh);
        void ProcessMeshlets(Mesh& mesh);
        u32 LoadMaterialTexture(Material& mat, const cgltf_texture_view* textureView, TextureType type);
        void ValidateResources() const;

        //ConstantBuffer cb;
        MaterialConstants matColor;

    public:
        std::string _dirPath;
        std::vector<Vertex> _vertices;
        std::vector<u32> _indices;
        std::vector<MeshInfo> _meshes;
        std::vector<Meshlet> _meshlets;
        std::vector<Material> _materials;

        std::shared_ptr<Renderer> _renderer;


        vk::Buffer _vertexBuffer = VK_NULL_HANDLE;
        vk::Buffer _indexBuffer = VK_NULL_HANDLE;
        vk::Buffer _meshletBuffer = VK_NULL_HANDLE;

        std::vector<Texture> modelTextures;
    private:


        vk::DeviceMemory _vertexMemory = VK_NULL_HANDLE;
        vk::DeviceMemory _indexMemory = VK_NULL_HANDLE;
        vk::DeviceMemory _meshletMemory = VK_NULL_HANDLE;

        std::unordered_set<std::string> loadedTextures; // To track loaded textures
        std::unordered_map<cgltf_material*, size_t> materialLookup;
        std::unordered_map<std::string, size_t> textureIndexLookup;

        ResourceManager* _resourceManager;

    };
}