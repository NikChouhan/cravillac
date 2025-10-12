#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_structs.hpp>

#include "common.h"

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR _capabilities;
    std::vector<vk::SurfaceFormatKHR> _formats;
    std::vector<vk::PresentModeKHR> _presentModes;
};

struct Queue
{
    vk::Queue _queue;
    u32 _index{0};
};

struct GfxDevice
{
    vk::Instance _instance;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::SurfaceKHR _surface;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    Queue _graphicsQueue;
    VmaAllocator _allocator;
    vk::CommandPool _commandPool;
};

struct GfxDeviceDesc
{
    bool _bEnableValidationLayers = true;
};

GfxDevice CreateDevice(GLFWwindow* window, GfxDeviceDesc desc);
void DestroyDevice(GfxDevice& gfxDevice);
std::vector<vk::CommandBuffer> CreateCommandBuffer(const GfxDevice& gfxDevice, u32 count);
void ImmediateSubmit(const GfxDevice& gfxDevice, LAMBDA(vk::CommandBuffer) callback);