#version 450

layout(set = 0, binding = 0) uniform UBO
{
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexcoord;

void main() 
{
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    fragTexcoord = inTexCoord;
}