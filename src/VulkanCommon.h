#pragma once

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
    #if defined(USE_WAYLAND)
        #define VK_USE_PLATFORM_WAYLAND_KHR
        #define GLFW_EXPOSE_NATIVE_WAYLAND
    #else
        #define VK_USE_PLATFORM_XLIB_KHR
        #define GLFW_EXPOSE_NATIVE_X11
    #endif
#endif

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    #include <vulkan/vulkan_wayland.h>
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    #include <vulkan/vulkan_win32.h>
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    #include <vulkan/vulkan_xlib.h>
#endif

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>