#version 450

layout(set = 0, binding = 0) uniform UBO
{
    mat4 mvp;
    mat4 mWorld;
} ubo;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inMaterialIndex;

layout(location = 0) out vec2 fragTexcoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out flat uint fragMaterialIndex;

void main() 
{
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    fragTexcoord = inTexCoord;
    fragNormal = mat3(ubo.mWorld) * inNormal;
    fragMaterialIndex = inMaterialIndex;        
}