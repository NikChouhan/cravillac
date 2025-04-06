#ifndef COMMON_H
#define COMMON_H

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

constexpr int MAX_FRAMES_IN_FLIGHT = 2;



#endif // COMMON_H