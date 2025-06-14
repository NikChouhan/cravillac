#ifndef VERTEX_H
#define VERTEX_H

#include "common.h"
#include <array>

namespace  Cravillac
{
    struct PushConstants
    {
        DirectX::XMMATRIX mvp;
        VkDeviceAddress vertexBufferAddress;
        DirectX::XMFLOAT3X3 normalMatrix;
        uint32_t materialIndex;
        float padding[2];
    };

    struct Vertex
    {
        SM::Vector3 pos;
        SM::Vector2 texCoord;
        SM::Vector3 normal;
    };
}
#endif