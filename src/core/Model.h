#pragma once

#include <unordered_map>
#include <unordered_set>

#include <cgltf.h>

#include "Buffer.h"
#include "GfxDevice.h"
#include "Texture.h"
#include "Vertex.h"

struct FrameSync;
struct Meshlet;
struct Texture;

struct ModelDesc
{
    std::string _path;
};

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
    bool _hasAlbedo = false;
    bool _hasNormal = false;
    bool _hasMetallicRoughness = false;
    bool _hasEmissive = false;
    bool _hasAO = false;

    u32 _albedoIndex = -1;
    u32 _normalIndex = -1;
    u32 _emmisiveIndex = -1;
    u32 _metallicIndex = -1;

    std::string _albedoPath;
    std::string _normalPath;
    std::string _metallicRoughnessPath;
    std::string _emissivePath;
    std::string _aOPath;

    vk::ImageView _albedoView = nullptr;
    vk::ImageView _normalView = nullptr;
    vk::ImageView _metallicRoughnessView = nullptr;
    vk::ImageView _emissiveView = nullptr;
    vk::ImageView _aOView = nullptr;

    DirectX::XMFLOAT3 _flatColor;
};

struct Mesh
{
    std::vector<Vertex> _vertices;
    std::vector<u32> _indices;
    u32 _vertexCount;
    u32 _indexCount;
};


struct MeshInfo
{
    size_t _vertexCount = 0;
    size_t _indexCount = 0;
    u32 _materialIndex = -1;
    uint32_t _startIndex = 0;
    uint32_t _startVertex = 0;
    Transformation _transform;
    glm::mat4 _normalMatrix;
};

struct Model
{
    std::string _dirPath{};
    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};
    std::vector<Meshlet> _meshlets;
    std::vector< MeshInfo> _meshes{};
    std::vector<Material> _materials{};

    Buffer _vertexBuffer;
    Buffer _indexBuffer;
    Buffer _meshletBuffer;

    vk::DeviceMemory _vertexMemory = VK_NULL_HANDLE;
    vk::DeviceMemory _indexMemory = VK_NULL_HANDLE;
    vk::DeviceMemory _meshletMemory = VK_NULL_HANDLE;

    std::vector<Texture> _modelTextures;

    std::unordered_set<std::string> _loadedTextures; // To track loaded textures
    std::unordered_map<cgltf_material*, size_t> _materialLookup;
    std::unordered_map<std::string, size_t> _textureIndexLookup;

    auto begin() { return _meshes.begin(); }
    auto end() { return _meshes.end(); }
};

Model LoadModel(GfxDevice& gfxDevice, FrameSync& frameSync, ModelDesc desc);
void DestroyModel(GfxDevice& gfxDevice, Model& model);
