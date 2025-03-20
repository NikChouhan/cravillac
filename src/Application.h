#pragma once
#include <vector>
#include <memory>
#include <array>
#include "glm/glm.hpp"

#include "common.h"

namespace Cravillac
{
	constexpr uint32_t WIDTH = 800;
	constexpr uint32_t HEIGHT = 600;

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDesc{
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };

            return bindingDesc;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 3> attrDescs{};

            // here binding is for the vertex position for a particular vertex buffer
            // means all the binding is for the same vertex buffer, with different locations for pos, color, texcoord
            attrDescs[0].binding = 0;
            attrDescs[0].location = 0;
            attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
            attrDescs[0].offset = offsetof(Vertex, pos);

            attrDescs[1].binding = 0;
            attrDescs[1].location = 1;
            attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrDescs[1].offset = offsetof(Vertex, color);

            attrDescs[2].binding = 0;
            attrDescs[2].location = 2;
            attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
            attrDescs[2].offset = offsetof(Vertex, texCoord);

            return attrDescs;
        }
    };

    const std::vector<Vertex> vertices =
    {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices =
    {
        0, 1, 2, 2, 3, 0
    };

    constexpr uint32_t MAX_TEXTURES = 3;

	class Renderer;
	class Texture;
	class ResourceManager;

	class Application
	{
	public:
		Application(const char* title);
		void Init();
		void Run();
		void DrawFrame();
		void SetResources();

	private:
		VkSurfaceKHR m_surface;
		std::shared_ptr<Renderer> renderer;
		GLFWwindow* m_window = nullptr;
		ResourceManager* m_resourceManager;

        VkBuffer m_vertexBuffer;
        VkBuffer m_indexBuffer;
        VkDeviceMemory m_vertexMemory;
        VkDeviceMemory m_indexMemory;
        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VkDeviceMemory> m_uniformBufferMem;
        std::vector<void*> m_uboMemMapped;

        std::vector<Texture>* textures;

	};
}