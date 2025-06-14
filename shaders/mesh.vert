#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
    vec3 pos;
    vec2 texCoord;
    vec3 normal;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant, scalar) uniform PushConstants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
    mat3 normalMatrix;
    uint materialIndex; // not used here
} pushConstants;

layout(location = 0) out vec2 fragTexcoord;
layout(location = 1) out vec3 fragNormal;

void main() 
{
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = pushConstants.mvp * vec4(v.pos, 1.0);
    fragTexcoord = v.texCoord;
    fragNormal = pushConstants.normalMatrix * v.normal;
}