#pragma once
#include <vector>
#include <memory>
#include <array>

#include "common.h"
#include "renderer.h"
#include "Vertex.h"

namespace Cravillac 
{
	class Renderer;
	class Texture;
	class ResourceManager;
    class PipelineManager;
}

struct ImGuiIO;

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

	class Application
	{
	public:
		Application(const char* title);
		void Init();
		void Run();
		void DrawFrame();
		void SetResources();

        void RecordCmdBuffer(VkCommandBuffer, uint32_t imageIndex);
        void UpdateUniformBuffer(uint32_t currentImage);

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

        std::vector<std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>> descriptorSets;

        std::vector<VkCommandBuffer> m_cmdBuffers;

        // sync primitives
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;

        std::vector<Texture>* textures;

        uint32_t currentFrame{ 0 };

        ImGuiIO* io;
	};
}