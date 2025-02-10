#include <string>
#include <iostream>
#include <set>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "renderer.h"
#include "Log.h"
#include <vk_utils.cpp>
#undef max;

namespace VKTest
{
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void Renderer::SetupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};

        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Debug Messenger Failure");
        }
        else Log::Info("[VULKAN] Debug messenger Success");
    }



    VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& code) const
    {
        VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };

        VkShaderModule shaderModule;

        if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            Log::Error("[SHADER] Shader Module creation Failed");
        }
        else Log::Info("[SHADER] Shader Module creation Success");

        return shaderModule;
    }


    void Renderer::Run()
    {
        Log::Init();
        InitWindow();
        InitVulkan();;
    }

    void Renderer::InitWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);
    }

    void Renderer::InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageView();
        CreateDesctriptorSetLayout();
        CreateGraphicsPipeline();
        CreateCommandPool();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateCommandBuffer();
        CreateSynObjects();
    }

    void Renderer::Cleanup() const
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphore[i], nullptr);
            vkDestroyFence(m_device, m_inFlightFence[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device, m_uniformBufferMemory[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
        }
        vkDestroyInstance(m_instance, nullptr);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }


    void Renderer::Render()
    {
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            DrawFrame();
        }

        vkDeviceWaitIdle(m_device);
    }

    void Renderer::CreateInstance()
    {
        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            Log::Error("[VULKAN] Validation layers requested but not available");
        }
        else Log::Info("[VULKAN] Validation layers requested available");

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Test",
            .applicationVersion = VK_API_VERSION_1_3,
            .pEngineName = "TestEngine",
            .engineVersion = VK_API_VERSION_1_3,
            .apiVersion = VK_API_VERSION_1_3
        };

        auto extensions = GetRequiredExtensions();

        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Instance creation Failed");
        }
        else Log::Info("[VULKAN] Instance creation Success");
    }

    void Renderer::CreateSurface()
    {
        // commented out cuz this is meant to be cross platform, and the hinstance and hwnd part only works for windows
        /*
        VkWin32SurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfwGetWin32Window(m_window),
        };

        if (vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Surface Creation Failure");
        }
        else Log::Info("[VULKAN] Surface Creation Success");*/

        // works across platforms
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Surface Creation Failure");
        }
        else Log::Info("[VULKAN] Surface Creation Success");
    }

    void Renderer::PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0) Log::Error("[VULKAN] Failed to find GPUs with Vulkan support");

        std::vector<VkPhysicalDevice> devices(deviceCount);

        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& physicalDevice : devices)
        {
            if (IsDeviceSuitable(physicalDevice, m_surface))
            {
                m_physicalDevice = physicalDevice;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) Log::Error("[VULKAN] Failed to find a suitable GPU");
        else {
            for (const auto& physdev : devices)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physdev, &properties);
                const char* name = properties.deviceName;
                Log::InfoDebug("[VULKAN] Device: ", name);
            }
        }
    }

    void Renderer::CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices._graphicsFamily.value(), indices._presentFamily.value() };

        float queuePriority = 1.f;
        for (auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceVulkan13Features enabledFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
        .dynamicRendering = true,
        };
        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &enabledFeatures,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
        };

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Logical Device creation Failure");
        }
        else Log::Info("[VULKAN] Logical Device creation Success");

        vkGetDeviceQueue(m_device, indices._graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices._presentFamily.value(), 0, &m_presentQueue);

    }

    void Renderer::CreateSwapChain()
    {
        assert(m_physicalDevice);
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(m_window, swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        //we do the imageCount > maximagecount check for following reason
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
            .surface = m_surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        };

        QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);
        uint32_t queueFamilyIndices[] = { indices._graphicsFamily.value(), indices._presentFamily.value() };

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
        else Log::Info("[VULKAN] SwapChain creation Success");

        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void Renderer::CreateImageView()
    {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_swapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_swapChainImageFormat,
            };
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            {
                Log::Error("[VULKAN] Image Views creation Failed");
            }
            else Log::Info("[VULKAN] Image Views creation Success");
        }
    }
    void Renderer::CreateDesctriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        };

        VkDescriptorSetLayoutCreateInfo pipelineLayoutCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboLayoutBinding
        };
        if (vkCreateDescriptorSetLayout(m_device, &pipelineLayoutCI, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Descriptor Layout creation Failed");
        }
        else Log::Info("[VULKAN] Descriptor Layout creation Success");
    }

    void Renderer::CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo descSetAllocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .pSetLayouts = layouts.data()
        };

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_device, &descSetAllocInfo, m_descriptorSets.data()) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Descriptor Sets creation Failed");
        } 
        else Log::Info("[VULKAN] Descriptor Sets creation Success");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // binding 0
            VkDescriptorBufferInfo descBI{
                .buffer = m_uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };

            VkWriteDescriptorSet writeDescSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET
            };
            writeDescSet.dstSet = m_descriptorSets[i];
            writeDescSet.descriptorCount = 1;
            writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescSet.pBufferInfo = &descBI;
            writeDescSet.dstBinding = 0;

            vkUpdateDescriptorSets(m_device, 1, &writeDescSet, 0, nullptr);
        }
    }


    // TODO
    //void Renderer::RecreateSwapChain()
    //{
    //    vkDeviceWaitIdle(m_device);

    //    CreateSwapChain();
    //    CreateImageView();
    //}

    void Renderer::CreateGraphicsPipeline()
    {
        auto vertShaderCode = ReadFile("../../../../shaders/Triangle.vert.spv");
        auto fragShaderCode = ReadFile("../../../../shaders/Triangle.frag.spv");

        Log::InfoDebug("[SHADER] Vert buffer Size: ", vertShaderCode.size());
        Log::InfoDebug("[SHADER] Frag buffer Size: ", fragShaderCode.size());

        VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

        // do stuff

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main"
        };

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
        };

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragShaderStageInfo };

        auto bindingDesc = Vertex::getBindingDescription();
        auto attribDesc = Vertex::getAttributeDescription();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
        vertexInputInfo.pVertexAttributeDescriptions = attribDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapChainExtent.width;
        viewport.height = (float)m_swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1; 
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout,
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            Log::Error("[VULKAN] Pipeline Layout creation Failure");
        }
        else Log::Info("[VULKAN] Pipeline Layout creation Success");

        VkPipelineRenderingCreateInfo pipelineRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_swapChainImageFormat,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        };

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Pipeline creation Failure");
        }
        else Log::Info("[VULKAN] Pipeline creation Success");

        // clean
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    }

    void Renderer::CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice, m_surface);

        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices._graphicsFamily.value()
        };

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Command Pool creation Failure");
        }
        else Log::Info("[VULKAN] Command Pool creation Success");
    }

    // TODO: when the project is big, crete a "CreateBuffer function which takes in as parameter, the "usage", buffer (like the vertices/indices in the future) for "size" entry, and create on basis of the type, orr simply take in enum type and create the type 
    void Renderer::CreateVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        CreateBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // What map does is : "When you call vkMapMemory, what gets mapped is a region of GPU memory (allocated via vkAllocateMemory) into the CPU's address space. This allows the CPU to directly access that GPU memory as if it were regular RAM."
        // the void data* (its stored in CPU address space or RAM (or caches and bla bla but for CPU remember) and it stores the gpu memory pointer(?) and is mapping of the GPU memory where the contents are copied
        void* data{};

        // imagine vkmapmemory as a function that takes in gpu memory and cpu memory pointer, map the memory and return a cpu pointer (void data*) to be used to copy stuf into it, which due to its mappingis also stored in the gpu memory
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), size_t(bufferSize));

        // (this releases and frees the data pointer implicitly after unmapping it ?) 
        vkUnmapMemory(m_device, stagingBufferMemory);

        CreateBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

        CopyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    void Renderer::CreateIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer{};
        VkDeviceMemory staginGBufferMem{};
        CreateBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, staginGBufferMem);

        void* data{};
        vkMapMemory(m_device, staginGBufferMem, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device, staginGBufferMem);

        CreateBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

        CopyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, staginGBufferMem, nullptr);
    }

    void Renderer::CreateUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBufferMemory[i]);
            vkMapMemory(m_device, m_uniformBufferMemory[i], 0, bufferSize, 0, &m_uniformBufferMapped[i]);
        }

    }

    void Renderer::CreateDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
        };
        VkDescriptorPoolCreateInfo descPoolCI{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        if (vkCreateDescriptorPool(m_device, &descPoolCI, nullptr, &m_descriptorPool) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Descriptor Pool creation Failed");
        }
        else Log::Info("[VULKAN] Descriptor Pool creation Success");
    }


    void Renderer::CreateCommandBuffer()
    {
        m_commandBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = (uint32_t)m_commandBuffer.size()
        };

        if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffer.data()) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Command Buffer creation Failure");
        }
        else Log::Info("[VULKAN] Command Buffer creation Success");
    }

    void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            Log::Error("[DRAW] Recording Command buffer Failure");
        }
        //else Log::Info("[DRAW] Recording Command buffer Success");

        // dynamic rendering stuff

        // transition color image from undefined to optimal for rendering
        TransitionImage(commandBuffer, m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);


        VkRenderingAttachmentInfo colorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_swapChainImageViews[imageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };
        colorAttachmentInfo.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        };
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = m_swapChainExtent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);


        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRendering(commandBuffer);

        TransitionImage(commandBuffer, m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            Log::Error("[DRAW] Recording Command buffer Failure");
        }
        //else Log::Info("[DRAW] Recording Command buffer Success");
    }

    void Renderer::DrawFrame()
    {
        vkWaitForFences(m_device, 1, &m_inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFence[currentFrame]);

        UpdateUniformBuffer(currentFrame);

        uint32_t imageIndex{};
        vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(m_commandBuffer[currentFrame], 0);

        RecordCommandBuffer(m_commandBuffer[currentFrame], imageIndex);


        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
        };
        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer[currentFrame];

        VkSemaphore signalSemaphore[] = { m_renderFinishedSemaphore[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphore;

        if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence[currentFrame]) != VK_SUCCESS)
        {
            Log::Error("[DRAW] Submit Draw Command buffer Failed");
        }
        //else Log::Info("[DRAW] Submit Draw Command buffer Success");

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapChain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(m_presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::UpdateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f);

        ubo.proj[1][1] *= -1;

        memcpy(m_uniformBufferMapped[currentImage], &ubo, sizeof(ubo));
    }

    void Renderer::CreateSynObjects()
    {
        m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);

        m_inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fenceCI{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_imageAvailableSemaphore[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_renderFinishedSemaphore[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFence[i]) != VK_SUCCESS) {
                Log::Error("[VULKAN] Fence/Semaphore creation Failure");
            }
            else Log::Info("[VULKAN] Fence/Semaphore creation Success");
        }
    }
};