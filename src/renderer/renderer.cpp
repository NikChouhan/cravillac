#include <mutex>
#include <iostream>
#include <set>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <fstream>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "renderer.h"
#include "Log.h"
#include "vk_utils.h"
#include <Texture.h>

namespace Cravillac
{
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void Renderer::SetupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};

        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Debug Messenger Failure");
        }
        else
            Log::Info("[VULKAN] Debug messenger Success");
    }

    VkShaderModule Renderer::CreateShaderModule(const std::vector<char> &code) const
    {
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data())};

        VkShaderModule shaderModule;

        if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            Log::Error("[SHADER] Shader Module creation Failed");
        }
        else
            Log::Info("[SHADER] Shader Module creation Success");

        return shaderModule;
    }

    Renderer::Renderer()
    {
    }


    void Renderer::InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
    }


    void Renderer::CreateInstance()
    {
        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            Log::Error("[VULKAN] Validation layers requested but not available");
        }
        else
            Log::Info("[VULKAN] Validation layers requested available");

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Test",
            .applicationVersion = VK_API_VERSION_1_3,
            .pEngineName = "TestEngine",
            .engineVersion = VK_API_VERSION_1_3,
            .apiVersion = VK_API_VERSION_1_3};

        auto extensions = GetRequiredExtensions();

        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Instance creation Failed");
        }
        else
            Log::Info("[VULKAN] Instance creation Success");
    }

    void Renderer::PickPhysicalDevice(VkSurfaceKHR surface)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            Log::Error("[VULKAN] Failed to find GPUs with Vulkan support");

        std::vector<VkPhysicalDevice> devices(deviceCount);

        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto &physicalDevice : devices)
        {
            if (IsDeviceSuitable(physicalDevice, surface))
            {
                m_physicalDevice = physicalDevice;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE)
            Log::Error("[VULKAN] Failed to find a suitable GPU");
        else
        {
            for (const auto &physdev : devices)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physdev, &properties);
                const char *name = properties.deviceName;
                Log::InfoDebug("[VULKAN] Device: ", name);
            }
        }
    }

    void Renderer::CreateLogicalDevice(VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices._graphicsFamily.value(), indices._presentFamily.value()};

        float queuePriority = 1.f;
        for (auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority};
            queueCreateInfos.push_back(queueCreateInfo);
        }
        // bindless
        VkPhysicalDeviceDescriptorIndexingFeatures bindless{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
        bindless.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        bindless.descriptorBindingPartiallyBound = VK_TRUE;
        bindless.runtimeDescriptorArray = VK_TRUE;
        bindless.descriptorBindingVariableDescriptorCount = VK_TRUE;
        bindless.descriptorBindingSampledImageUpdateAfterBind = true;
        // dynamic rendering
        VkPhysicalDeviceVulkan13Features enabledFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &bindless,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE,
        };
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &enabledFeatures,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
        };

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Logical Device creation Failure");
        }
        else
            Log::Info("[VULKAN] Logical Device creation Success");

        vkGetDeviceQueue(m_device, indices._graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices._presentFamily.value(), 0, &m_presentQueue);
    }


    void Renderer::CreateCommandPool(VkSurfaceKHR surface)
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice, surface);

        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices._graphicsFamily.value() };

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Command Pool creation Failure");
        }
        else
            Log::Info("[VULKAN] Command Pool creation Success");
    }

    void Renderer::CreateSwapChain(VkSurfaceKHR surface, GLFWwindow* window)
    {
        assert(m_physicalDevice);
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(window, swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // we do the imageCount > maximagecount check for following reason
        /*  For example :
            Let�s say minImageCount = 2 (double buffering is the minimum requirement).
            You set imageCount = 3 (triple buffering for better performance).
            But the platform only supports a maximum of 2 images(maxImageCount = 2).
            In this case, imageCount = 3 would exceed maxImageCount = 2, which is not allowed.*/

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        assert(m_device);
        VkSwapchainCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

        QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, surface);
        uint32_t queueFamilyIndices[] = {indices._graphicsFamily.value(), indices._presentFamily.value()};

        if (indices._graphicsFamily != indices._presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] SwapChain creation Failed");
        }
        else
            Log::Info("[VULKAN] SwapChain creation Success");

        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;

        // swapchain image views
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            m_swapChainImageViews[i] = CreateImageView(m_device, m_swapChainImages[i], m_swapChainImageFormat);
        }
    }

    // TODO
    // void Renderer::RecreateSwapChain()
    //{
    //    vkDeviceWaitIdle(m_device);

    //    CreateSwapChain();
    //    CreateImageView();
    //}


    void Renderer::CreateCommandBuffer(std::vector<VkCommandBuffer>& cmdBuffers) const
    {
        cmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(cmdBuffers.size())};

        if (vkAllocateCommandBuffers(m_device, &allocInfo, cmdBuffers.data()) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Command Buffer creation Failure");
        }
        else
            Log::Info("[VULKAN] Command Buffer creation Success");
    }

    void Renderer::CreateSynObjects(std::vector<VkSemaphore>& imgAvailableSem, std::vector<VkSemaphore>& renderFinishedSem, std::vector<VkFence>& inFlightFences)
    {
        imgAvailableSem.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSem.resize(MAX_FRAMES_IN_FLIGHT);

        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fenceCI{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &imgAvailableSem[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &renderFinishedSem[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fenceCI, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                Log::Error("[VULKAN] Fence/Semaphore creation Failure");
            }
            else
                Log::Info("[VULKAN] Fence/Semaphore creation Success");
        }
    }
};