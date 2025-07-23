#ifndef VERTEX_H
#define VERTEX_H

#include <vulkan/vulkan.hpp>

#include "common.h"
#include <SimpleMath.h>
#include <glm/glm.hpp>

#include "StandardTypes.h"

namespace SM = DirectX::SimpleMath;

namespace  CV
{
    struct PushConstants
    {
        glm::mat4 mvp;
        vk::DeviceAddress vertexBufferAddress;
#if MESH_SHADING
        VkDeviceAddress meshletBufferAddress;
#endif
        glm::mat3 normalMatrix;
        u32 materialIndex;
        float padding[4];
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 texCoord;
        glm::vec3 normal;
    };

    struct Meshlet
    {
        u32 vertices[64];
        u8 indices[126];
        u8 triangleCount;
        u8 vertexCount;
    };
}
#endif