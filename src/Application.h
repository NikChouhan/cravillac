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
	constexpr uint32_t WIDTH = 1280;
	constexpr uint32_t HEIGHT = 720;


    struct UniformBufferObject
    {
        DirectX::XMMATRIX mvp;
        DirectX::XMFLOAT3X3 normalMatrix;
    };

    class Application
    {
    public:
        explicit Application();
        void Init();
        void Run();
        void DrawFrame();
        void SetResources();

        void RecordCmdBuffer(VkCommandBuffer, uint32_t imageIndex, uint32_t currentFrame) const;
        [[nodiscard]] UniformBufferObject UpdateUniformBuffer(const MeshInfo& meshInfo) const;


        std::shared_ptr<Cravillac::Camera> m_camera;

    private:
        const char *title;
        VkSurfaceKHR m_surface;
        std::shared_ptr<Renderer> renderer;
        GLFWwindow* m_window = nullptr;
        ResourceManager* m_resourceManager;
        // bda handle
        VkDeviceAddress m_vertexBufferAddress;
        VkDeviceAddress m_meshletBufferAddress;

        std::array<std::vector<VkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

        std::vector<VkCommandBuffer> m_cmdBuffers{ VK_NULL_HANDLE };

        // sync primitives
        std::vector<VkSemaphore> m_imageAvailableSemaphore{ VK_NULL_HANDLE };
        std::vector<VkSemaphore> m_renderFinishedSemaphore{ VK_NULL_HANDLE };
        std::vector<VkFence> m_inFlightFence{ VK_NULL_HANDLE };

        std::vector<Texture> textures;
        uint32_t m_currentFrame{ 0 };
        std::vector<Model> models;

	};
}