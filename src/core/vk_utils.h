#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <vector>
#include <optional>
#include <string>
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include "StandardTypes.h"

namespace CV
{
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
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	// Vulkan base support functions
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	bool IsDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);
	vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D ChooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);
	std::optional<uint32_t> FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	// Transition image layout for rendering/presenting, etc
	void TransitionImage(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout);
	void CreateBuffer(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags propertyFlags, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void CopyBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	vk::CommandBuffer BeginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool);
	void EndSingleTimeCommands(vk::Device device, vk::Queue queue, vk::CommandPool commandPool, vk::CommandBuffer commandBuffer);

	// Image handling
	void CreateImage(vk::PhysicalDevice physicalDevice, vk::Device device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& memory);
	void CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Buffer& buffer, uint32_t width, uint32_t height);
	vk::ImageView CreateImageView(vk::Device device, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);

	// Depth and stencil image ops
	vk::Format FindSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags flags);
	vk::Format FindDepthFormat(vk::PhysicalDevice physicalDevice);
	bool HasStencilComponent(vk::Format format);

	// Read shader file
	std::vector<char> ReadShaderFile(const std::string& filename);
}
#endif