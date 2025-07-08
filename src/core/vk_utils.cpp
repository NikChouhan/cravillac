#include <pch.h>

#include <fstream>
#include <set>
#include <algorithm>

#include "vk_utils.h"

#include "Log.h"

#ifdef _WIN32
#undef max
#endif

namespace Cravillac
{
    bool CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers{layerCount};
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (auto &layer : validationLayers)
        {
            bool layerFound = false;
            for (const auto &available : availableLayers)
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

    std::vector<const char *> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool IsDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
        const bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
        bool swapChainAdequate = false;

        if (extensionsSupported)
        {
            printl(Log::LogLevel::Info,"[VULKAN] Required Extensions supported!");
            const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.IsComplete() && extensionsSupported && swapChainAdequate;
    }
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice)
    {
        auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            // Log::InfoDebug("[VULKAN] Queue Family: ", static_cast<uint32_t>(queueFamily.queueFlags));

            bool presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);

            if (presentSupport)
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

    SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
        details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        return details;
    }

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

    vk::Extent2D ChooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities)
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

    std::optional<uint32_t> FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        auto memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        printl(Log::LogLevel::Error,"[MEMORY] Failed to find suitable memory type");
        return std::nullopt;
    }

    void TransitionImage(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout)
    {
        vk::ImageAspectFlags aspectMask{ 0 };
	    vk::PipelineStageFlags2 srcStageMask{};
	    vk::PipelineStageFlags2 dstStageMask{};
        vk::AccessFlags2 srcAccessMask{ 0 };
        vk::AccessFlags2 dstAccessMask{ 0 };

        if (newLayout == vk::ImageLayout::eColorAttachmentOptimal)
        {
            aspectMask = vk::ImageAspectFlagBits::eColor;
            srcAccessMask = {};
            srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        }
        else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            aspectMask = vk::ImageAspectFlagBits::eDepth; // or with vk::ImageAspectFlagBits::eStencil later when want to enable
            srcAccessMask = {};
            srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
            dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        }
        else
        {
            aspectMask = vk::ImageAspectFlagBits::eColor;    // this is for the texture transitions
        }

        vk::ImageMemoryBarrier2 imageBarrier{};
        imageBarrier.sType = vk::StructureType::eImageMemoryBarrier2;
        imageBarrier.pNext = nullptr;

        vk::ImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1; // not using mipmaps
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1; // For a standard 2D image

        if (currentLayout == vk::ImageLayout::eUndefined && (
            newLayout == vk::ImageLayout::eColorAttachmentOptimal ||
            newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal))
        {
            // access mask - how the resource is being used
            // stage mask - at what stage the ops need to be done
            imageBarrier.srcAccessMask = {};                   // no memory access for source (no previous memory operations to sync with)
            imageBarrier.srcStageMask = srcStageMask;
            imageBarrier.dstAccessMask = dstAccessMask;
            imageBarrier.dstStageMask = dstStageMask;         // after the barrier finishes start the transfer at the transfer stage
        }
        // texture transition
        else if (currentLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            // access mask - how the resource is being used
            // stage mask - at what stage the ops need to be done
            imageBarrier.srcAccessMask = {};                                     // no memory access for source (no previous memory operations to sync with)
            imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;      // start at the top of pipeline stage
            imageBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;          // enable transfer write ops 
            imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;         // after the barrier finishes start the transfer at the transfer stage
        }
        // texture transition again
        else if (currentLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            imageBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;          // the previous (src) operation is being done, so wait it out
            imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;         // start at the transfer stage (transfer ops are handled by separate units in a physical gpu)
            imageBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;             // enable access of shader for read
            imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;  // after the barrier finishes start the ops (transition here) at the fragment stage in pipeline
        }
        else if (currentLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR)
        {
            imageBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            imageBarrier.dstAccessMask = {};
            imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
        }
        else
        {
            // temporary solution, unoptimised but atleast works
            imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
            imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
        }

        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;
        imageBarrier.subresourceRange = subresourceRange;
        imageBarrier.image = image;

        vk::DependencyInfo depInfo{};
        depInfo.sType = vk::StructureType::eDependencyInfo;
        depInfo.pNext = nullptr;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        commandBuffer.pipelineBarrier2(depInfo);
    }
    // redundant
    void CreateBuffer(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize size,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags propertyFlags,
        vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
    {
        // what's happening here is simple af. Create a bufferCI with appropriate size , then create a buffer.
        // Then make memrequirements struct, to store the memrequirements, supplied with the buffer.
        // Then allocate memory using the memrequirements, and store it in memory.
        // Then bind the memory, map it to cpu accessible buffer

        vk::BufferCreateInfo bufferCI{};
        bufferCI.size = size;
        bufferCI.usage = usage;
        bufferCI.sharingMode = vk::SharingMode::eExclusive;

        try
        {
            buffer = device.createBuffer(bufferCI);
            //printl(Log::LogLevel::Info,"[VULKAN] Vertex Buffer creation Success");
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error, "[VULKAN] Vertex Buffer creation Failure: {}", std::string(err.what()));
            return;
        }

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);
        auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, propertyFlags);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 0;  // Will be set below

        if (memoryTypeIndex.has_value())
        {
            allocInfo.memoryTypeIndex = memoryTypeIndex.value();
        }
        else
        {
            printl(Log::LogLevel::Error, "[VULKAN] No memory type found");
            return;
        }

        try
        {
            bufferMemory = device.allocateMemory(allocInfo);
            //printl(Log::LogLevel::Info, "[VULKAN] Allocation of Vertex Buffer Memory Success");
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error, "[VULKAN] Allocation of Vertex Buffer Memory Failed: {}", std::string(err.what()));
            return;
        }

        device.bindBufferMemory(buffer, bufferMemory, 0);
    }
    void CopyBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
    {
        vk::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
    }

    vk::CommandBuffer BeginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool)
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        auto commandBuffers = device.allocateCommandBuffers(allocInfo);
        auto commandBuffer = commandBuffers[0];

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(vk::Device device, vk::Queue queue, vk::CommandPool commandPool, vk::CommandBuffer commandBuffer)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.sType = vk::StructureType::eSubmitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        auto result = queue.submit(1, &submitInfo, nullptr);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to copy buffer to image: " + vk::to_string(result));
        }
        queue.waitIdle(); // very very inefficient. TODO: Implement memory barrier instead

        device.freeCommandBuffers(commandPool, 1, &commandBuffer);
    }

    void CreateImage(vk::PhysicalDevice physicalDevice, vk::Device device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory)
    {
        vk::ImageCreateInfo imageCI{};
        imageCI.sType = vk::StructureType::eImageCreateInfo;
        imageCI.imageType = vk::ImageType::e2D;
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.extent.width = static_cast<uint32_t>(width);
        imageCI.extent.height = static_cast<uint32_t>(height);
        imageCI.extent.depth = 1;
        imageCI.format = format;
        imageCI.tiling = tiling;
        imageCI.initialLayout = vk::ImageLayout::eUndefined;
        imageCI.usage = usage;
        imageCI.sharingMode = vk::SharingMode::eExclusive;
        imageCI.samples = vk::SampleCountFlagBits::e1;
        imageCI.flags = {};

        auto result = device.createImage(&imageCI, nullptr, &image);
        if (result != vk::Result::eSuccess)
        {
            printl(Log::LogLevel::Error,"[STB] Failed to Create Texture Image");
        }
        else
        {
            //printl(Log::LogLevel::Info, "[STB] Success to Create Texture Image");
        }

        auto memRequirements = device.getImageMemoryRequirements(image);
        auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
        allocInfo.allocationSize = memRequirements.size;

        if (memoryTypeIndex.has_value())
            allocInfo.memoryTypeIndex = memoryTypeIndex.value();
        else
            printl(Log::LogLevel::Error,"[VULKAN] No memory type found");

        auto allocResult = device.allocateMemory(&allocInfo, nullptr, &imageMemory);
        if (allocResult != vk::Result::eSuccess)
        {
            printl(Log::LogLevel::Error,"[TEXTURE] Failed to allocate texture memory");
        }
        // else printl(Log::LogLevel::Info, "[TEXTURE] Success to allocate texture memory");

        device.bindImageMemory(image, imageMemory, 0);
    }


    void CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Buffer& buffer, uint32_t width, uint32_t height)
    {
        vk::BufferImageCopy2 region{};
        region.sType = vk::StructureType::eBufferImageCopy2;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{ 0, 0, 0 };
        region.imageExtent = vk::Extent3D{ width, height, 1 };

        vk::CopyBufferToImageInfo2 bufferCI{};
        bufferCI.sType = vk::StructureType::eCopyBufferToImageInfo2;
        bufferCI.srcBuffer = buffer;
        bufferCI.regionCount = 1;
        bufferCI.pRegions = &region;
        bufferCI.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        bufferCI.dstImage = image;

        commandBuffer.copyBufferToImage2(&bufferCI);
    }

    vk::ImageView CreateImageView(vk::Device device, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags)
    {
        vk::ImageSubresourceRange range{};
        range.aspectMask = aspectFlags;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        vk::ImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = vk::StructureType::eImageViewCreateInfo;
        imageViewCI.image = image;
        imageViewCI.viewType = vk::ImageViewType::e2D;
        imageViewCI.format = format;
        imageViewCI.subresourceRange = range;

        vk::ImageView imageView;
        auto result = device.createImageView(&imageViewCI, nullptr, &imageView);
        if (result != vk::Result::eSuccess)
        {
            printl(Log::LogLevel::Error,"[TEXTURE] Failed to create Image View");
            imageView = nullptr;
        }
        else
        {
            //printl(Log::LogLevel::Info,"[TEXTURE] Success to create Image View");
        }
        return imageView;
    }

    vk::Format FindSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags flags)
    {
        for (vk::Format format : candidates)
        {
            auto props = physicalDevice.getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && ((props.linearTilingFeatures & flags) == flags))
                return format;
            if (tiling == vk::ImageTiling::eOptimal && ((props.optimalTilingFeatures & flags) == flags))
                return format;
        }
        throw std::runtime_error("Failed to find supported format");
    }

    vk::Format FindDepthFormat(vk::PhysicalDevice physicalDevice)
    {
        return FindSupportedFormat(physicalDevice,
            { vk::Format::eD32Sfloat },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    bool HasStencilComponent(vk::Format format)
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    std::vector<char> ReadShaderFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            printl(Log::LogLevel::Error,"[SHADER] Failed to open file");
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
}