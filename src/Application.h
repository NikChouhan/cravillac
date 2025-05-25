#pragma once
#include <vector>
#include <memory>
#include <array>

#include "common.h"
#include "Vertex.h"
#include "Model.h"

namespace Cravillac 
{
    class Texture;
	class Renderer;
	class ResourceManager;
    class PipelineManager;
    class Camera;
}

struct ImGuiIO;

namespace Cravillac
{
	constexpr uint32_t WIDTH = 1920;
	constexpr uint32_t HEIGHT = 1080;


    struct UniformBufferObject
    {
        DirectX::XMMATRIX mvp;
        DirectX::XMFLOAT3X3 normalMatrix;
    };

    class Application
    {
    public:
        Application(const char* title);
        void Init();
        void Run();
        void DrawFrame();
        void SetResources();

        void RecordCmdBuffer(VkCommandBuffer, uint32_t imageIndex, uint32_t currentFrame) const;
        UniformBufferObject UpdateUniformBuffer(uint32_t currentImage, const Primitive& prim) const;


        std::shared_ptr<Cravillac::Camera> m_camera;

    private:
        const char* title;
        VkSurfaceKHR m_surface;
        std::shared_ptr<Renderer> renderer;
        GLFWwindow* m_window = nullptr;
        ResourceManager* m_resourceManager;

        VkBuffer m_vertexBuffer {VK_NULL_HANDLE};
        VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_vertexMemory{ VK_NULL_HANDLE };
        VkDeviceMemory m_indexMemory{ VK_NULL_HANDLE };
        std::vector<VkBuffer> m_uniformBuffers{ VK_NULL_HANDLE };
        std::vector<VkDeviceMemory> m_uniformBufferMem{ VK_NULL_HANDLE };
        std::vector<void*> m_uboMemMapped{ VK_NULL_HANDLE };

        std::array<std::vector<VkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

        std::vector<VkCommandBuffer> m_cmdBuffers{ VK_NULL_HANDLE };

        // sync primitives
        std::vector<VkSemaphore> m_imageAvailableSemaphore{ VK_NULL_HANDLE };
        std::vector<VkSemaphore> m_renderFinishedSemaphore{ VK_NULL_HANDLE };
        std::vector<VkFence> m_inFlightFence{ VK_NULL_HANDLE };

        std::vector<Texture> textures;

        // material index storage buffer
        std::vector<VkBuffer> m_matIndexSSBO;
        std::vector<VkDeviceMemory> m_matIndexSSBOMemory;

        uint32_t currentFrame{ 0 };

        std::vector<Model> models;

	};
}