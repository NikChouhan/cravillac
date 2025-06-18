#include <vector>
#include <string>
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

    bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

	        const bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            Log::Info("[VULKAN] Required Extensions supported!");
            const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.IsComplete() && extensionsSupported && swapChainAdequate;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        VkBool32 presentSupport = false;

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            // Log::InfoDebug("[VULKAN] Queue Family: ", queueFamily.queueFlags);

            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices._presentFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices._graphicsFamily = i;

                if (indices.IsComplete())
                    break;
            }
            i++;
        }

        return indices;
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
        uint32_t formatCount{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return availablePresentMode;
            }
        }
        return availablePresentModes[0];
    }

    VkExtent2D ChooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.height);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    std::optional<u32> FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        Log::Error("[MEMORY] Failed to find suitable memory type");
        return std::nullopt;

    }

    void TransitionImage(VkCommandBuffer commandBuffer, VkImage &image, VkImageLayout currentLayout,
                         VkImageLayout newLayout)
    {
        VkImageAspectFlags aspectMask{0};
        VkPipelineStageFlags2 srcStageMask{0};
        VkPipelineStageFlags2 dstStageMask{0};
        VkAccessFlags2 srcAccessMask{0};
        VkAccessFlags2 dstAccessMask{0};
        if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            srcAccessMask = 0;
            srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // or with VK_IMAGE_ASPECT_STENCIL_BIT later when want to enable
            srcAccessMask = 0;
            srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }
        else aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // this is for the texture transitions

        VkImageMemoryBarrier2 imageBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr};
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1; // not using mipmaps
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1; // For a standard 2D image

        if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && (
                newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
        {
            // access mask - how the resource is being used
            // stage mask - at what stage the ops need to be done
            imageBarrier.srcAccessMask = 0;                   // no memory access for source (no previous memory operations to sync with)
            imageBarrier.srcStageMask = srcStageMask;
            imageBarrier.dstAccessMask = dstAccessMask;  
            imageBarrier.dstStageMask = dstStageMask;         // after the barrier finishes start the transfer at the transfer stage
        }
        // texture transition
        else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            // access mask - how the resource is being used
            // stage mask - at what stage the ops need to be done
            imageBarrier.srcAccessMask = 0;                                     // no memory access for source (no previous memory operations to sync with)
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;      // start at the top of pipeline stage
            imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;          // enable transfer write ops 
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;         // after the barrier finishes start the transfter at the transfer stage
        }
        // texture transition again
        else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout ==
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;          // the previous (src) operation is being done, so wait it out
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;         // start at the transfer stage (transfer ops are handled by separate units in a physical gpu)
            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;             // enable access of shader for read
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // after the barrier finishes start the ops (transition here) at the fragment stage in pipeline
        }
        else if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            imageBarrier.dstAccessMask = 0;
        	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        }
        else
        {
            // temporary solution, unoptimised but atleast works
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
        }

        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;

        imageBarrier.subresourceRange = subresourceRange;
        imageBarrier.image = image;

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;

        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(commandBuffer, &depInfo);
    }
    // redundant
    void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags propertyFlags, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        // what's happening here is simple af. Create a bufferCI with appropriate size , then create a buffer.
        // Then make memrequirements struct, to store the memrequirements, supplied with the buffer.
        // Then allocate memory using the memrequirements, and store it in memory.
        // Then bind the memory, map it to cpu accessible buffer
        //
        VkBufferCreateInfo bufferCI{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        if (vkCreateBuffer(device, &bufferCI, nullptr, &buffer) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Vertex Buffer creation Failure");
        }
        else
            Log::Info("[VULKAN] Vertex Buffer creation Success");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, propertyFlags);

        VkMemoryAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        if (memoryTypeIndex.has_value())
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, propertyFlags).value();
        else Log::Error("[VULKAN] No memory type found");


        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Allocation of Vertex Buffer Memory Failed");
        }
        else
            Log::Info("[VULKAN] Allocation of Vertex Buffer Memory Success");
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void CopyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize size)
    {
        const VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size};

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
    }

    VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue); // very very inefficient.TODO: Implement memory barrier instead

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void CreateImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .mipLevels = 1,
            .arrayLayers = 1};

        imageCI.extent.width = static_cast<uint32_t>(width);
        imageCI.extent.height = static_cast<uint32_t>(height);
        imageCI.extent.depth = 1;

        imageCI.format = format;
        imageCI.tiling = tiling;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = usage;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.flags = 0;

        if (vkCreateImage(device, &imageCI, nullptr, &image) != VK_SUCCESS)
        {
            Log::Error("[STB] Failed to Create Texture Image");
        }
        else
            Log::Info("[STB] Success to Create Texture Image");

        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        auto memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        VkMemoryAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        if (memoryTypeIndex.has_value())
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties).value();
        else Log::Error("[VULKAN] No memory type found");

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            Log::Error("[TEXTURE] Failed to allocate texture memory");
        }
        else
            Log::Info("[TEXTURE] Success to allocate texture memory");

        vkBindImageMemory(device, image, imageMemory, 0);
    }


    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkImage& image, VkBuffer& buffer, uint32_t width, uint32_t height)
    {
        VkBufferImageCopy2 region{
            .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        VkCopyBufferToImageInfo2 bufferCI{.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
        bufferCI.srcBuffer = buffer;
        bufferCI.regionCount = 1;
        bufferCI.pRegions = &region;
        bufferCI.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bufferCI.dstImage = image;
        vkCmdCopyBufferToImage2(commandBuffer, &bufferCI);
    }


    VkImageView CreateImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageSubresourceRange range{
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        VkImageViewCreateInfo imageViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = range,
        };

        VkImageView imageView;

        if(vkCreateImageView(device, &imageViewCI, nullptr, &imageView) != VK_SUCCESS)
        {
            Log::Error("[TEXTURE] Failed to create Image View");
            imageView = VK_NULL_HANDLE;
        }
        else 
        {
            Log::Info("[TEXTURE] Success to create Image View");
        }
        return imageView;
    }

    VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
	    VkFormatFeatureFlags flags)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && ((props.linearTilingFeatures & flags) == flags))
                return format;
            if (tiling == VK_IMAGE_TILING_OPTIMAL && ((props.optimalTilingFeatures & flags) == flags))
                return format;
        }
        throw std::runtime_error("Failed to find supported format");
    }


    VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
	{
        return FindSupportedFormat(physicalDevice,
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    std::vector<char> ReadShaderFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            Log::Error("[SHADER] Failed to open file");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}