#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <vector>
#include <optional>
#include <string>

namespace Cravillac
{
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> _graphicsFamily;
		std::optional<uint32_t> _presentFamily;

		bool IsComplete() const {
			return _graphicsFamily.has_value() && _presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// vulkan base support funcs
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	// transition image layout for rendering/presenting, etc
	void TransitionImage(VkCommandBuffer commandBuffer, VkImage& image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize size);
	VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	void EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

	// Image handling
	void CreateImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);
	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkImage& image, VkBuffer& buffer, uint32_t width, uint32_t height);
    VkImageView CreateImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags);

	//depth and stencil image ops
	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags flags);
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
	bool HasStencilComponent(VkFormat format);

	// read file idk shader
	std::vector<char> ReadShaderFile(const std::string &filename);

}
#endif