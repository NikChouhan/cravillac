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

#include <SimpleMath.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
inline uint32_t MAX_TEXTURES = 256;

namespace SM = DirectX::SimpleMath;

// error check

#define VK_ASSERT(call)                                                                 \
    do                                                                                 \
    {                                                                                  \
        VkResult result = call;                                                        \
        if (result != VK_SUCCESS)                                                      \
        {                                                                              \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", result, __FILE__, __LINE__); \
            abort();                                                                   \
        }                                                                              \
    } while (0)

#endif // COMMON_H