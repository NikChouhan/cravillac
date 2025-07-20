#pragma once
#include <vector>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "pch.h"


namespace CV
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
    public:
        vk::Instance _instance;
        vk::PhysicalDevice _physicalDevice;
        vk::Device _device;
        vk::Queue _graphicsQueue;
        vk::Queue _presentQueue;
        vk::SwapchainKHR _swapChain;
        vk::Format _swapChainImageFormat;
        vk::Extent2D _swapChainExtent;
        std::vector<vk::Image> _swapChainImages{};
        std::vector<vk::ImageView> _swapChainImageViews{};
        //depth image vars
        vk::Image _depthImage;
        vk::DeviceMemory _depthImageMemory;
        vk::ImageView _depthImageView;
        vk::Format _depthImageFormat;
        std::vector<vk::Buffer> _uniformBuffers{};
        std::vector<vk::DeviceMemory> _uniformBufferMemory{};
        std::vector<void*> _uniformBufferMapped{};
        // issue commands
        vk::CommandPool _commandPool;
        std::vector<vk::CommandBuffer> _commandBuffer;
        // sync primitives
        vk::DebugUtilsMessengerEXT _debugMessenger;
    };
};