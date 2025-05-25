#version 450

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    mat3 normalMatrix;
    uint materialIndex; // not used here
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexcoord;
layout(location = 1) out vec3 fragNormal;

void main() 
{
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);
    fragTexcoord = inTexCoord;
    fragNormal = pushConstants.normalMatrix * inNormal;
}