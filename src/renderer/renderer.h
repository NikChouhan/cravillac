#pragma once

#define GL_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <array>

#include "common.h"


namespace Cravillac
{
    class Texture;

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    enum class Buffer
    {
        VERTEX,
        INDEX
    };

    

    class Renderer
    {
    public:
        Renderer();
        void InitVulkan();
        void Submit(std::vector<Texture>& textures);
        void Render();
        void Cleanup() const;

        void CreateSwapChain();

        // Vulkan base setup
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateDesctriptorSetLayout();
        void CreateDescriptorSets(std::vector<Texture>& textures);

        // void RecreateSwapChain(); // TODO

        // drawing stuff
        void CreateGraphicsPipeline();
        void CreateCommandPool();
        void CreateVertexBuffer();
        void CreateIndexBuffer();
        void CreateUniformBuffers();
        void CreateDescriptorPool();
        void CreateCommandBuffer();
        void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void DrawFrame();

        // update stuff
        void UpdateUniformBuffer(uint32_t currentImage);

        // fence stuff
        void CreateSynObjects();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void SetupDebugMessenger();
        VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    public:
        GLFWwindow *m_window;
        uint32_t currentFrame = 0;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImage> m_swapChainImages{};
        std::vector<VkImageView> m_swapChainImageViews{};
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

        // bufffer vars
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

        std::vector<VkBuffer> m_uniformBuffers{};
        std::vector<VkDeviceMemory> m_uniformBufferMemory{};
        std::vector<void *> m_uniformBufferMapped{};

        // descriptor pool
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptorSets;

        // issue commands
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffer;

        // sync primitives
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;

        VkDebugUtilsMessengerEXT debugMessenger;
    };
};