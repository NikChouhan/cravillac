#pragma once
#include <vector>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "pch.h"


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
        ~Renderer() = default;
        void InitVulkan();
        void CreateSwapChain(vk::SurfaceKHR surface, GLFWwindow* window);
        // Vulkan base setup
        void CreateInstance();
        void PickPhysicalDevice(vk::SurfaceKHR surface);
        void CreateLogicalDevice(vk::SurfaceKHR surface);
        void CreateCommandPool(vk::SurfaceKHR surface);
        void CreateDepthResources();
        // void RecreateSwapChain(); // TODO
        // drawing stuff
        void CreateCommandBuffer(std::vector<vk::CommandBuffer>& cmdBuffers) const;
        // fence stuff
        void CreateSynObjects(std::vector<vk::Semaphore>& imgAvailableSem, std::vector<vk::Semaphore>& renderFinishedSem, std::vector<vk::Fence>& inFlightFences);
        void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
        void SetupDebugMessenger();
        [[nodiscard]] vk::ShaderModule CreateShaderModule(const std::vector<char>& code) const;
    public:
        vk::Instance m_instance;
        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_device;
        vk::Queue m_graphicsQueue;
        vk::Queue m_presentQueue;
        vk::SwapchainKHR m_swapChain;
        vk::Format m_swapChainImageFormat;
        vk::Extent2D m_swapChainExtent;
        std::vector<vk::Image> m_swapChainImages{};
        std::vector<vk::ImageView> m_swapChainImageViews{};
        //depth image vars
        vk::Image m_depthImage;
        vk::DeviceMemory m_depthImageMemory;
        vk::ImageView m_depthImageView;
        vk::Format m_depthImageFormat;
        std::vector<vk::Buffer> m_uniformBuffers{};
        std::vector<vk::DeviceMemory> m_uniformBufferMemory{};
        std::vector<void*> m_uniformBufferMapped{};
        // issue commands
        vk::CommandPool m_commandPool;
        std::vector<vk::CommandBuffer> m_commandBuffer;
        // sync primitives
        vk::DebugUtilsMessengerEXT debugMessenger;
    };
};