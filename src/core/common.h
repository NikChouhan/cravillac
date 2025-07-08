#ifndef COMMON_H
#define COMMON_H

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
inline uint32_t MAX_TEXTURES = 256;
#define EXTREME 1

// meshInfo shading pipeline
#define MESH_SHADING 0

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

#endif // COMMON_H