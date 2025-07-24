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
        u32 albedoIndex;
        u32 normalIndex;
        u32 metallicIndex;
        u32 emissiveIndex;
        float padding;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec4 tangent;
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

// KNOW THAT the lights in the scene are emmisive materials