#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include <DirectXMath.h>

#include <cgltf.h>

struct SimpleVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 TexCoord;
	DirectX::XMFLOAT3 Normal;
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

struct Primitive
{
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	std::string name = "";
	uint32_t materialIndex = 0;
	uint32_t startIndex = 0;
	uint32_t startVertex = 0;

	Transformation transform;
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

	// texture view here. For vukan a vkimage and a vkimageview
	// 
	//
	VkImage AlbedoImage = VK_NULL_HANDLE;
	VkImageView AlbedoView = VK_NULL_HANDLE;
	VkImage NormalImage = VK_NULL_HANDLE;
	VkImageView NormalView = VK_NULL_HANDLE;
	VkImage MetallicRoughnessImage = VK_NULL_HANDLE;
	VkImageView MetallicRoughnessView = VK_NULL_HANDLE;
	VkImage EmissiveImage = VK_NULL_HANDLE;
	VkImageView EmissiveView = VK_NULL_HANDLE;
	VkImage AOImage = VK_NULL_HANDLE;
	VkImageView AOView = VK_NULL_HANDLE;

	DirectX::XMFLOAT3 FlatColor;

	VkDeviceMemory AlbedoMemory = VK_NULL_HANDLE;
	VkDeviceMemory NormalMemory = VK_NULL_HANDLE;
	VkDeviceMemory MetallicRoughnessMemory = VK_NULL_HANDLE;
	VkDeviceMemory EmissiveMemory = VK_NULL_HANDLE;
	VkDeviceMemory AOMemory = VK_NULL_HANDLE;
};

namespace Cravillac
{
	class Renderer;
}

namespace Cravillac
{
	class Model
	{
	public:
		Model();
		void LoadModel(std::shared_ptr<Renderer> renderer, std::string path);
	private:
		void ProcessNode(cgltf_node* node, const cgltf_data* data, Transformation& parentTransform);
		void ProcessPrimitive(cgltf_primitive* primitive, const cgltf_data* data, Transformation& parentTransform);
		void SetVertIndBuffers();
	public:
		std::vector<SimpleVertex> m_vertices{};
		std::vector<uint32_t> m_indices{};
		std::vector<Primitive> m_primitives{};
	private:
		std::shared_ptr<Renderer> renderer;
		std::string m_dirPath{};

		VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
		VkBuffer m_indexBuffer = VK_NULL_HANDLE;

		VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
	};
}

#endif