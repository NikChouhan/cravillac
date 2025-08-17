#include "Swapchain.h"

#include <glm/ext/vector_common.hpp>
#undef max;

static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}
static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eImmediate)
        {
            return availablePresentMode;
        }
    }
    return availablePresentModes[0];
}
static vk::Extent2D ChooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

static SwapChainSupportDetails QuerySwapChainSupport(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;

    details._capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details._formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details._presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    return details;
}

static vk::SwapchainKHR CreateSwapchain(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, vk::SurfaceFormatKHR surfaceFormat, vk::PresentModeKHR presentMode, vk::Extent2D extent,
                    vk::SwapchainKHR oldSwapchain, u32 preferresSwImageCount)
{
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    u32 minImageCount = glm::clamp(preferresSwImageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
	vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1u;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwapchain;

    vk::SwapchainKHR swapchain = device.createSwapchainKHR(createInfo);

    return swapchain;
}

Swapchain CreateSwapchain(GfxDevice& gfxDevice, GLFWwindow* window, SwapchainDesc& desc)
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(gfxDevice._physicalDevice, gfxDevice._surface);
    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport._formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupport._presentModes);
    vk::Extent2D extent = ChooseSwapExtent(window, swapChainSupport._capabilities);

    Swapchain swapchain;
    swapchain._extent = extent;
    swapchain._format = surfaceFormat.format;

    if (desc._vsyncOption == true)
        swapchain._presentMode = vk::PresentModeKHR::eFifo;
    else swapchain._presentMode = presentMode;

    swapchain._swapchain = CreateSwapchain(gfxDevice._device, gfxDevice._physicalDevice, gfxDevice._surface,
                                           surfaceFormat, presentMode, extent, desc._oldSwapchain,
                                           desc._preferresSWImageCount);

    std::vector<vk::Image> swapchainImages = gfxDevice._device.getSwapchainImagesKHR(swapchain._swapchain);
    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        swapchain._textures.push_back(CreateTexture(gfxDevice, {
        	._width = extent.width,
            ._height = extent.height,
            ._format = surfaceFormat.format,
            ._layout = vk::ImageLayout::ePresentSrcKHR,
            ._access = vk::AccessFlags(0),
            ._resource = swapchainImages[i] }));
    }
    return swapchain;
}
