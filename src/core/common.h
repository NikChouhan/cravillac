#ifndef COMMON_H
#define COMMON_H

#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_enums.hpp>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "StandardTypes.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
inline u32 MAX_TEXTURES = 1024;
#define EXTREME 0

// meshInfo shading pipeline
#define MESH_SHADING 0

// array size
template <typename T, size_t N>
constexpr size_t ArraySize(T(&)[N]) { return N; }

// error check

#define VK_ASSERT(call)                                                                                     \
    do                                                                                                      \
    {                                                                                                       \
        vk::Result result = call;                                                                           \
        if (result != vk::Result::eSuccess)                                                                 \
        {                                                                                                   \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", static_cast<int>(result), __FILE__, __LINE__);    \
			abort();                                                                                        \
        }                                                                                                   \
    } while (0)
// assert with lambda
#define VK_ASSERT_L(call, cleanupLambda)                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        vk::Result result = call;                                                                           \
        if (result != vk::Result::eSuccess)                                                                 \
        {                                                                                                   \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", static_cast<int>(result), __FILE__, __LINE__);    \
			cleanupLambda();                                                                                \
			abort();                                                                                        \
        }                                                                                                   \
    } while (0)


// assert for ResultValue types
// this is not needed at all because VulkanHpp has exceptions enabled
// although yes, this assert was only half implemented before I got to know about
// it. Kept for knowledge reasons idk.
#define VK_ASSERT_RV(call)                                                                                     \
    do                                                                                                      \
    {                                                                                                       \
        auto result = call;                                                                           \
        if (result.result != vk::Result::eSuccess)                                                                 \
        {                                                                                                   \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", static_cast<int>(result), __FILE__, __LINE__);    \
			abort();                                                                                        \
        }                                                                                                   \
    } while (0)

// function pointer thingy
#define LAMBDA(...) std::function<void(__VA_ARGS__)> const&

#endif // COMMON_H