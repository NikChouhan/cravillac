#pragma once


#include <vector>
#include "Model.h"
#include "common.h"

namespace Cravillac
{
    class Texture;

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

        void CreateSwapChain(VkSurfaceKHR surface, GLFWwindow* window);

        // Vulkan base setup
        void CreateInstance();
        void PickPhysicalDevice(VkSurfaceKHR surface);
        void CreateLogicalDevice(VkSurfaceKHR surface);
        void CreateCommandPool(VkSurfaceKHR surface);

        // void RecreateSwapChain(); // TODO

        // drawing stuff
        void CreateCommandBuffer(std::vector<VkCommandBuffer>& cmdBuffers) const;


        // fence stuff
        void CreateSynObjects(std::vector<VkSemaphore>& imgAvailableSem, std::vector<VkSemaphore>& renderFinishedSem, std::vector<VkFence>& inFlightFences);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void SetupDebugMessenger();
        VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    public:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImage> m_swapChainImages{};
        std::vector<VkImageView> m_swapChainImageViews{};


        std::vector<VkBuffer> m_uniformBuffers{};
        std::vector<VkDeviceMemory> m_uniformBufferMemory{};
        std::vector<void *> m_uniformBufferMapped{};

        // issue commands
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffer;

        // sync primitives

        VkDebugUtilsMessengerEXT debugMessenger;
    };
};