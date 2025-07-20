#include <pch.h>

#include <iostream>
#include <set>
#include <cstdint>
#include <chrono>

#include "renderer.h"
#include "Log.h"
#include "vk_utils.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace CV
{
    static HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        WORD color = FOREGROUND_RED | FOREGROUND_INTENSITY;

        switch (messageSeverity)
        {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        default:
            break;
        }

        SetConsoleTextAttribute(hConsole, color);

        std::cerr << "[Validation][" << vk::to_string(messageSeverity) << "] "
            << "[" << pCallbackData->pMessageIdName << " | " << pCallbackData->messageIdNumber << "] "
            << pCallbackData->pMessage << "\n\n";

        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Reset to default gray
#if _WIN32
#if EXTREME
        if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            __debugbreak(); // breaks execution here, letting you inspect the call stack
#endif
#endif
        return vk::False;
    }


    void Renderer::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
        createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        createInfo.pfnUserCallback = debugCallback;
    }

    void Renderer::SetupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        try
        {
            _debugMessenger = _instance.createDebugUtilsMessengerEXT(createInfo);
            printl(Log::LogLevel::Info,"[VULKAN] Debug messenger Success");
        }
        catch (const vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Debug Messenger Failure: {}", std::string(err.what()));
            throw;
        }
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
        // Initialize the dynamic loader
        vk::detail::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

        // Initialize the default dispatcher with the function pointer
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            printl(Log::LogLevel::Error,"[VULKAN] Validation layers requested but not available");
        }
        else
            printl(Log::LogLevel::Info,"[VULKAN] Validation layers requested available");

        auto appInfo = vk::ApplicationInfo{};
        appInfo.pApplicationName = "Vulkan Test";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // App version, not API version
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        auto extensions = GetRequiredExtensions();
        auto instanceInfo = vk::InstanceCreateInfo{};
        instanceInfo.setPApplicationInfo(&appInfo).setPEnabledExtensionNames(extensions);

        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};  // Fixed syntax
        if (enableValidationLayers)
        {
            instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instanceInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            instanceInfo.pNext = &debugCreateInfo;
        }

        // Create instance - no need to pass dispatcher here
        _instance = vk::createInstance(instanceInfo);

        // Initialize the dispatcher with the instance
        VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);
    }

    void Renderer::PickPhysicalDevice(vk::SurfaceKHR surface)
    {
        std::vector<vk::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();

        if (devices.empty())
            printl(Log::LogLevel::Error,"[VULKAN] Failed to find GPUs with Vulkan support");
        for (const auto& physicalDevice : devices)
        {
            if (IsDeviceSuitable(physicalDevice, surface))
            {
                _physicalDevice = physicalDevice;
                break;
            }
        }

        if (!_physicalDevice)  // vk::PhysicalDevice has implicit bool conversion
            printl(Log::LogLevel::Error,"[VULKAN] Failed to find a suitable GPU");
        else
        {
            for (const auto& physdev : devices)
            {
                auto properties = physdev.getProperties();
                const char* name = properties.deviceName;
                printl(Log::LogLevel::InfoDebug,"[VULKAN] Device: {}", name);
            }
        }
    }
    void Renderer::CreateLogicalDevice(vk::SurfaceKHR surface)
    {
        QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice, surface);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices._graphicsFamily.value(), indices._presentFamily.value() };

        float queuePriority = 1.0f;
        for (auto queueFamily : uniqueQueueFamilies)
        {
            vk::DeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // vk dynamic rendering local read
        /* vk::PhysicalDeviceDynamicRenderingLocalReadFeatures drLocalRead{};
        drLocalRead.dynamicRenderingLocalRead = vk::True; */

        // mesh shading (optional)
        vk::PhysicalDeviceMeshShaderFeaturesNV meshShaderFeatures{};
        meshShaderFeatures.meshShader = vk::True;

        // BDA and scalar layout
        vk::PhysicalDeviceScalarBlockLayoutFeatures scalarFeatures{};
        scalarFeatures.scalarBlockLayout = vk::True;
#if MESH_SHADING
        scalarFeatures.pNext = &meshShaderFeatures;
#endif

        vk::PhysicalDeviceBufferDeviceAddressFeatures bdaFeatures{};
        bdaFeatures.bufferDeviceAddress = vk::True;
        bdaFeatures.bufferDeviceAddressCaptureReplay = vk::True;
        bdaFeatures.pNext = &scalarFeatures;

#if MESH_SHADING
        vk::PhysicalDeviceMeshShaderPropertiesNV meshShaderProperties{};

        vk::PhysicalDeviceProperties2 props{};
        props.pNext = &meshShaderProperties;

        _physicalDevice.getProperties2(&props);
#endif

        // bindless
        vk::PhysicalDeviceDescriptorIndexingFeatures bindless{};
        // bindless.pNext = &drLocalRead;
        bindless.pNext = &bdaFeatures;
        bindless.shaderSampledImageArrayNonUniformIndexing = vk::True;
        bindless.descriptorBindingPartiallyBound = vk::True;
        bindless.runtimeDescriptorArray = vk::True;
        bindless.descriptorBindingVariableDescriptorCount = vk::True;
        bindless.descriptorBindingSampledImageUpdateAfterBind = vk::True;

        // dynamic rendering
        vk::PhysicalDeviceVulkan13Features enabledFeatures;
        enabledFeatures.pNext = &bindless;
        enabledFeatures.synchronization2 = vk::True;
        enabledFeatures.dynamicRendering = vk::True;

        vk::PhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = vk::True;
        deviceFeatures.fragmentStoresAndAtomics = vk::True;

        vk::DeviceCreateInfo createInfo{};
    	createInfo.pNext = &enabledFeatures;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        try
        {
            _device = _physicalDevice.createDevice(createInfo);
            printl(Log::LogLevel::Info,"[VULKAN] Logical Device creation Success");
        }
        catch (const vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Logical Device creation Failure: {} ", std::string(err.what()));
            throw;
        }

        _graphicsQueue = _device.getQueue(indices._graphicsFamily.value(), 0);
        _presentQueue = _device.getQueue(indices._presentFamily.value(), 0);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);
    }

    void Renderer::CreateCommandPool(vk::SurfaceKHR surface)
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(_physicalDevice, surface);

        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices._graphicsFamily.value();

        try
        {
            // Use _device.createCommandPool (C++ API) instead of vkCreateCommandPool (C API)
            _commandPool = _device.createCommandPool(poolInfo);
            printl(Log::LogLevel::Info,"[VULKAN] Command Pool creation Success");
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Command Pool creation Failure: {}", std::string(err.what()));
        }
    }

    void Renderer::CreateDepthResources()
    {
        _depthImageFormat = FindDepthFormat(_physicalDevice);
        CreateImage(_physicalDevice, _device, _swapChainExtent.width, _swapChainExtent.height,
            _depthImageFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal, _depthImage, _depthImageMemory);
        _depthImageView = CreateImageView(_device, _depthImage, _depthImageFormat, vk::ImageAspectFlagBits::eDepth);
    }

    void Renderer::CreateSwapChain(vk::SurfaceKHR surface, GLFWwindow* window)
    {
        assert(_physicalDevice);
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(_physicalDevice, surface);
        auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        auto presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        vk::Extent2D extent = ChooseSwapExtent(window, swapChainSupport.capabilities);
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // we do the imageCount > maximagecount check for following reason
        /*  For example :
            Let's say minImageCount = 2 (double buffering is the minimum requirement).
            You set imageCount = 3 (triple buffering for better performance).
            But the platform only supports a maximum of 2 images(maxImageCount = 2).
            In this case, imageCount = 3 would exceed maxImageCount = 2, which is not allowed.*/
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        assert(_device);
        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1u;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

        QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice, surface);
        uint32_t queueFamilyIndices[] = { indices._graphicsFamily.value(), indices._presentFamily.value() };

        if (indices._graphicsFamily != indices._presentFamily)
        {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = vk::True;
        createInfo.oldSwapchain = nullptr;

        try
        {
            _swapChain = _device.createSwapchainKHR(createInfo);
            printl(Log::LogLevel::Info,"[VULKAN] SwapChain creation Success");
        }
        catch (const vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] SwapChain creation Failed: {}", std::string(err.what()));
            throw;
        }

        _swapChainImages = _device.getSwapchainImagesKHR(_swapChain);
        _swapChainImageFormat = surfaceFormat.format;
        _swapChainExtent = extent;

        // swapchain image views
        _swapChainImageViews.resize(_swapChainImages.size());
        for (size_t i = 0; i < _swapChainImages.size(); i++)
        {
            _swapChainImageViews[i] = CreateImageView(_device, _swapChainImages[i], _swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
        }
    }

    // TODO
    // void Renderer::RecreateSwapChain()
    //{
    //    vkDeviceWaitIdle(_device);

    //    CreateSwapChain();
    //    CreateImageView();
    //}

    void Renderer::CreateCommandBuffer(std::vector<vk::CommandBuffer>& cmdBuffers) const
    {
        cmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());

        try
        {
            cmdBuffers = _device.allocateCommandBuffers(allocInfo);
            printl(Log::LogLevel::Info,"[VULKAN] Command Buffer creation Success");
        }
        catch (const vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Command Buffer creation Failure: {} ", std::string(err.what()));
            throw;
        }
    }

    void Renderer::CreateSynObjects(std::vector<vk::Semaphore>& imgAvailableSem, std::vector<vk::Semaphore>& renderFinishedSem, std::vector<vk::Fence>& inFlightFences)
    {
        imgAvailableSem.resize(MAX_FRAMES_IN_FLIGHT);
        // only renderFinishedSem is resized to swapchain images size because it is used in two queues, it is signalled from graphics queue
        // when render is finished, and waited by present queue. If we index it with current frame parameter which is just cpu side fence parameter
        // it wont work. In the cpu code the current frame parameter is changed in the end of the loop, and its asynchronous to the gpu rendering and
        // present code, so its possible that the current frame parameter changes before the presentation is done. Remember that the current frame
        // var was used to index into the renderFinishedSem, so its possible (and highly likely for gpu driven work, where cpu is more idle than the gpu)
        // for it to index into the wrong semaphore, and signal an already signalled semaphore.
        renderFinishedSem.resize(_swapChainImages.size());
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreCI{};

        vk::FenceCreateInfo fenceCI{};
        fenceCI.flags = vk::FenceCreateFlagBits::eSignaled;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {

        	try
            {
                imgAvailableSem[i] = _device.createSemaphore(semaphoreCI);
                inFlightFences[i] = _device.createFence(fenceCI);

                printl(Log::LogLevel::Info,"[VULKAN] Fence/Semaphore creation Success for frame {} ", std::to_string(i));
            }
            catch (const vk::SystemError& err)
            {
                printl(Log::LogLevel::Error,"[VULKAN] Fence/Semaphore creation Failure for frame {}, error: {}", std::to_string(i), std::string(err.what()));
                throw;
            }
        }
        for (int i = 0; i < _swapChainImages.size(); i++)
        {

            try
            {
                renderFinishedSem[i] = _device.createSemaphore(semaphoreCI);
                printl(Log::LogLevel::Info,"[VULKAN] Semaphore renderfinished creation success");
            }
            catch (const vk::SystemError& err)
            {
                std::string error = err.what();
                printl(Log::LogLevel::Error,"[VULKAN] Semaphore renderfinished creation failure {}",error);
            }
        }
    }
}