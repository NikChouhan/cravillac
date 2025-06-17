#ifndef VERTEX_H
#define VERTEX_H

#include "common.h"
#include <array>

#include "StandardTypes.h"

namespace  Cravillac
{
    struct PushConstants
    {
        DirectX::XMMATRIX mvp;
        VkDeviceAddress vertexBufferAddress;
#if MESH_SHADING
        VkDeviceAddress meshletBufferAddress;
#endif
        DirectX::XMFLOAT3X3 normalMatrix;
        u32 materialIndex;
        float padding[2];
    };

    struct Vertex
    {
        SM::Vector3 pos;
        SM::Vector2 texCoord;
        SM::Vector3 normal;
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