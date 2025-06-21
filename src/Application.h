#pragma once
#include <vector>
#include <memory>
#include <array>

#include "common.h"
#include "Vertex.h"
#include "Model.h"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

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
        void InitFrame();
        void EndFrame();
        void Init();
        void Run();
        void DrawFrame();
        void SetResources();

        void RecordCmdBuffer(vk::CommandBuffer, uint32_t imageIndex) const;
        [[nodiscard]] UniformBufferObject UpdateUniformBuffer(const MeshInfo& meshInfo) const;

        vk::Device GetDevice();

        std::shared_ptr<Cravillac::Camera> m_camera;

    private:
        const char *title;
        vk::SurfaceKHR m_surface;
        std::shared_ptr<Renderer> renderer;
        vk::Device device{};
        GLFWwindow* m_window = nullptr;
        ResourceManager* m_resourceManager;
        // bda handle
        vk::DeviceAddress m_vertexBufferAddress;
        vk::DeviceAddress m_meshletBufferAddress;

        std::array<std::vector<vk::DescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

        std::vector<vk::CommandBuffer> m_cmdBuffers{ VK_NULL_HANDLE };

        // sync primitives
        std::vector<vk::Semaphore> m_imageAvailableSemaphore{ VK_NULL_HANDLE };
        std::vector<vk::Semaphore> m_renderFinishedSemaphore{ VK_NULL_HANDLE };
        std::vector<vk::Fence> m_inFlightFence{ VK_NULL_HANDLE };

        std::vector<Texture> textures;
        uint32_t m_currentFrame{ 0 };
        std::vector<Model> models;

	};
}