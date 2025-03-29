#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main() 
{
    outColor = texture(textures[0], fragTexcoord);
}