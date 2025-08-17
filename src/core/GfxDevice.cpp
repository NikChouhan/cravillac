#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "GfxDevice.h"
#include <iostream>
#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

using Log = CV::Log;
static HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);

struct QueueFamilyIndices
{
    std::optional<uint32_t> _graphicsFamily;
    std::optional<uint32_t> _presentFamily;

    bool IsComplete() const {
        return _graphicsFamily.has_value() && _presentFamily.has_value();
    }
};

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME,
#if MESH_SHADING
        VK_NV_MESH_SHADER_EXTENSION_NAME
#endif
};


static SwapChainSupportDetails QuerySwapChainSupport(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;

    details._capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details._formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details._presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    return details;
}

static QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices;

    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // Log::InfoDebug("[VULKAN] Queue Family: ", static_cast<uint32_t>(queueFamily.queueFlags));
        if (bool presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface))
        {
            indices._presentFamily = i;
        }
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices._graphicsFamily = i;

            if (indices.IsComplete())
                break;
        }
        i++;
    }
    return indices;
}

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
static void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
    createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

static bool CheckValidationLayerSupport(const std::vector<const char*> validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers{ layerCount };
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto& layer : validationLayers)
    {
        bool layerFound = false;
        for (const auto& available : availableLayers)
        {
            if (strcmp(available.layerName, layer) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;
    }
    return true;
}

static std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice)
{
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static vk::Instance CreateInstance(bool enableValidationLayers)
{
    vk::Instance instance;
    const std::vector validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME,
    #if MESH_SHADING
            VK_NV_MESH_SHADER_EXTENSION_NAME
    #endif
    };

    // Initialize the dynamic loader
    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

    // Initialize the default dispatcher with the function pointer
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if (enableValidationLayers && !CheckValidationLayerSupport(validationLayers))
    {
        printl(CV::Log::LogLevel::Error, "[VULKAN] Validation layers requested but not available");
    }
    else
	    printl(CV::Log::LogLevel::Info, "[VULKAN] Validation layers requested available");

    auto appInfo = vk::ApplicationInfo{};
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // App version, not API version
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = GetRequiredExtensions(enableValidationLayers);
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
    instance = vk::createInstance(instanceInfo);

    // Initialize the dispatcher with the instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    return instance;
}
static vk::DebugUtilsMessengerEXT SetupDebugMessenger(vk::Instance instance)
{
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    try
    {
        debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
        printl(Log::LogLevel::Info, "[VULKAN] Debug messenger Success");
    }
    catch (const vk::SystemError& err)
    {
        printl(Log::LogLevel::Error, "[VULKAN] Debug Messenger Failure: {}", std::string(err.what()));
        throw;
    }
    return debugUtilsMessenger;
}

static vk::SurfaceKHR CreateSurface(GLFWwindow* window, vk::Instance instance)
{
    VkSurfaceKHR ret{};
    auto res = (glfwCreateWindowSurface(instance, window, nullptr, &ret));
    if (res != VK_SUCCESS)
        printl(Log::LogLevel::Error, "[VULKAN] GLFW Window surface");
    else printl(Log::LogLevel::Info, "[VULKAN] GLFW window surface");
    vk::SurfaceKHR surface = vk::SurfaceKHR{ ret };

    return surface;
}

static vk::PhysicalDevice PickPhysicalDevice(const vk::Instance instance, const vk::SurfaceKHR surface)
{
    vk::PhysicalDevice physicalDevice;
    const std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    if (devices.empty())
        printl(Log::LogLevel::Error, "[VULKAN] Failed to find GPUs with Vulkan support");
    auto isDeviceSuitable = [&](const vk::PhysicalDevice& physDevice)
        {
            const QueueFamilyIndices indices = FindQueueFamilies(physDevice, surface);
            const bool extensionsSupported = CheckDeviceExtensionSupport(physDevice);
            bool swapChainAdequate = false;

            if (extensionsSupported)
            {
                printl(Log::LogLevel::Info, "[VULKAN] Required Extensions supported!");
                const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physDevice, surface);
                swapChainAdequate = !swapChainSupport._formats.empty() && !swapChainSupport._presentModes.empty();
            }

            return indices.IsComplete() && extensionsSupported && swapChainAdequate;
        };

    for (const auto& psd : devices)
    {
        if (isDeviceSuitable(psd))
        {
            physicalDevice = psd;
            break;
        }
    }

    if (!physicalDevice)  // vk::PhysicalDevice has implicit bool conversion
        printl(Log::LogLevel::Error, "[VULKAN] Failed to find a suitable GPU");
    else
    {
        for (const auto& physdev : devices)
        {
            auto properties = physdev.getProperties();
            const char* name = properties.deviceName;
            printl(Log::LogLevel::InfoDebug, "[VULKAN] Device: {}", name);
        }
    }
    return physicalDevice;
}

static vk::Device CreateDevice(vk::PhysicalDevice physicalDevice, u32 queueFamilyIndex, bool enableValidationLayers)
{
    vk::Device device;

    float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;


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

#if MESH_SHADING
    vk::PhysicalDeviceMeshShaderPropertiesNV meshShaderProperties{};

    vk::PhysicalDeviceProperties2 props{};
    props.pNext = &meshShaderProperties;

    _physicalDevice.getProperties2(&props);
#endif

    // bindless
    vk::PhysicalDeviceDescriptorIndexingFeatures bindless{};
    // bindless.pNext = &drLocalRead;
    bindless.shaderSampledImageArrayNonUniformIndexing = vk::True;
    bindless.descriptorBindingPartiallyBound = vk::True;
    bindless.runtimeDescriptorArray = vk::True;
    bindless.descriptorBindingVariableDescriptorCount = vk::True;
    bindless.descriptorBindingSampledImageUpdateAfterBind = vk::True;

    // dynamic rendering
    vk::PhysicalDeviceVulkan13Features enabledFeatures;
    enabledFeatures.synchronization2 = vk::True;
    enabledFeatures.dynamicRendering = vk::True;

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = vk::True;
    deviceFeatures.fragmentStoresAndAtomics = vk::True;
    deviceFeatures.shaderInt64 = vk::True;

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
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
    createInfo.pNext = &enabledFeatures;
    enabledFeatures.pNext = &bindless;
    bindless.pNext = &bdaFeatures;
    bdaFeatures.pNext = &scalarFeatures;

    try
    {
        device = physicalDevice.createDevice(createInfo);
        printl(Log::LogLevel::Info, "[VULKAN] Logical Device creation Success");
    }
    catch (const vk::SystemError& err)
    {
        printl(Log::LogLevel::Error, "[VULKAN] Logical Device creation Failure: {} ", std::string(err.what()));
        throw;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    return device;
}

static VmaAllocator CreateAllocator(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device)
{
    const auto& d = VULKAN_HPP_DEFAULT_DISPATCHER;

    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = d.vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = d.vkGetDeviceProcAddr;
    vulkanFunctions.vkGetPhysicalDeviceProperties = d.vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = d.vkGetPhysicalDeviceMemoryProperties;
    vulkanFunctions.vkAllocateMemory = d.vkAllocateMemory;
    vulkanFunctions.vkFreeMemory = d.vkFreeMemory;
    vulkanFunctions.vkMapMemory = d.vkMapMemory;
    vulkanFunctions.vkUnmapMemory = d.vkUnmapMemory;
    vulkanFunctions.vkFlushMappedMemoryRanges = d.vkFlushMappedMemoryRanges;
    vulkanFunctions.vkInvalidateMappedMemoryRanges = d.vkFlushMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory = d.vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory = d.vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements = d.vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements = d.vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer = d.vkCreateBuffer;
    vulkanFunctions.vkDestroyBuffer = d.vkDestroyBuffer;
    vulkanFunctions.vkCreateImage = d.vkCreateImage;
    vulkanFunctions.vkDestroyImage = d.vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer = d.vkCmdCopyBuffer;
    vulkanFunctions.vkGetBufferMemoryRequirements2KHR = d.vkGetBufferMemoryRequirements2;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR = d.vkGetImageMemoryRequirements2;
    vulkanFunctions.vkBindBufferMemory2KHR = d.vkBindBufferMemory2;
    vulkanFunctions.vkBindImageMemory2KHR = d.vkBindImageMemory2;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = d.vkGetPhysicalDeviceMemoryProperties2;
    vulkanFunctions.vkGetDeviceBufferMemoryRequirements = d.vkGetDeviceBufferMemoryRequirements;
    vulkanFunctions.vkGetDeviceImageMemoryRequirements = d.vkGetDeviceImageMemoryRequirements;


    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    VK_ASSERT((static_cast<vk::Result>(vmaCreateAllocator(&allocatorCreateInfo, &allocator))));

    return allocator;
}

static vk::CommandPool CreateCommandPool(const vk::Device device, const u32 queueFamilyIndex)
{
	vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;   

    const vk::CommandPool commandPool = device.createCommandPool(commandPoolCreateInfo, nullptr);
    assert(commandPool);
    return commandPool;
}

GfxDevice CreateDevice(GLFWwindow* window, const GfxDeviceDesc desc)
{
	GfxDevice gfxDevice{};
	gfxDevice._instance = CreateInstance(desc._bEnableValidationLayers);
    gfxDevice._debugMessenger = SetupDebugMessenger(gfxDevice._instance);
    gfxDevice._surface = CreateSurface(window, gfxDevice._instance);
    gfxDevice._physicalDevice = PickPhysicalDevice(gfxDevice._instance, gfxDevice._surface);
    assert(gfxDevice._physicalDevice);

    gfxDevice._graphicsQueue._index = FindQueueFamilies(gfxDevice._physicalDevice, gfxDevice._surface)._graphicsFamily.value();

    gfxDevice._device = CreateDevice(gfxDevice._physicalDevice, gfxDevice._graphicsQueue._index, desc._bEnableValidationLayers);
    gfxDevice._graphicsQueue._queue = gfxDevice._device.getQueue(gfxDevice._graphicsQueue._index, 0);
    gfxDevice._allocator = CreateAllocator(gfxDevice._instance, gfxDevice._physicalDevice, gfxDevice._device);
    gfxDevice._commandPool = CreateCommandPool(gfxDevice._device, gfxDevice._graphicsQueue._index);
	return gfxDevice;
}

void DestroyDevice(GfxDevice& gfxDevice)
{
    gfxDevice._device.destroyCommandPool(gfxDevice._commandPool);
    vmaDestroyAllocator(gfxDevice._allocator);
    vkDestroyDevice(gfxDevice._device, nullptr);

    if (gfxDevice._debugMessenger != nullptr)
    {
        gfxDevice._instance.destroyDebugUtilsMessengerEXT(gfxDevice._debugMessenger);
    }
    gfxDevice._instance.destroySurfaceKHR(gfxDevice._surface);
    vkDestroyInstance(gfxDevice._instance, nullptr);

    gfxDevice = {};
}

std::vector<vk::CommandBuffer> CreateCommandBuffer(const GfxDevice& gfxDevice, const u32 count)
{
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = gfxDevice._commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = count;

    std::vector<vk::CommandBuffer> commandBuffers;
    commandBuffers.resize(count);

    commandBuffers = gfxDevice._device.allocateCommandBuffers(commandBufferAllocateInfo);

    return commandBuffers;
}

void ImmediateSubmit(const GfxDevice& gfxDevice, std::function<void(vk::CommandBuffer)> const& callback)
{
    vk::CommandBuffer commandBuffer = CreateCommandBuffer(gfxDevice, 1).at(0);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    callback(commandBuffer);

    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

	VK_ASSERT_L(gfxDevice._graphicsQueue._queue.submit(1, &submitInfo, nullptr),
        [&]()
        {
            gfxDevice._device.freeCommandBuffers(gfxDevice._commandPool, 1, &commandBuffer);
        });
    gfxDevice._graphicsQueue._queue.waitIdle();
    gfxDevice._device.freeCommandBuffers(gfxDevice._commandPool, 1, &commandBuffer);
}