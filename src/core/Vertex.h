#ifndef VERTEX_H
#define VERTEX_H

#include "common.h"

#include <array>
#include <vector>

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

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
        // means all the binding is for the same vertex buffer, with different locations for pos, color, texcoord
        attrDescs[0].binding = 0;
        attrDescs[0].location = 0;
        attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrDescs[0].offset = offsetof(Vertex, pos);

        attrDescs[1].binding = 0;
        attrDescs[1].location = 1;
        attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDescs[1].offset = offsetof(Vertex, color);

        attrDescs[2].binding = 0;   
        attrDescs[2].location = 2;
        attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrDescs[2].offset = offsetof(Vertex, texCoord);

        return attrDescs;
    }
};
#endif