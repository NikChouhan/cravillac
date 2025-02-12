#ifndef COMMON_HPP
#define COMMON_HPP

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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Log.h"


#endif // COMMON_HPP