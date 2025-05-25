#ifndef VERTEX_H
#define VERTEX_H

#include "common.h"
#include <array>

namespace  Cravillac
{
    struct PushConstants
    {
        DirectX::XMMATRIX mvp;
        DirectX::XMFLOAT3X3 normalMatrix;
        float padding[3];   
        uint32_t materialIndex;
    };

    struct Vertex
    {
        SM::Vector3 pos;
        SM::Vector2 texCoord;
        SM::Vector3 normal;
        
        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDesc{
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };

            return bindingDesc;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 3> attrDescs{};

            // here binding is for the vertex position for a particular vertex buffer
            // means all the binding is for the same vertex buffer, with different locations for pos, texcoord, normal
            attrDescs[0].binding = 0;
            attrDescs[0].location = 0;
            attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrDescs[0].offset = offsetof(Vertex, pos);

            attrDescs[1].binding = 0;
            attrDescs[1].location = 1;
            attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
            attrDescs[1].offset = offsetof(Vertex, texCoord);

            attrDescs[2].binding = 0;
            attrDescs[2].location = 2;
            attrDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrDescs[2].offset = offsetof(Vertex, normal);

            return attrDescs;
        }
    };
}
#endif