#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexcoord;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform PushConstants
{
    mat4 mvp;   // not used here, but in vertex shader
    mat3 normalMatrix;  // not used here, but in vertex shader
    uint materialIndex;
} pushConstants;

void main() 
{

    vec3 ambientColor = vec3(0.2f, 0.2f, 0.2f);
    vec3 diffuseColor = vec3(0.8f, 0.8f, 0.8f);

    vec3 normal = normalize(fragNormal);

    // Basic lighting direction (can be moved to constant buffer)
    vec3 lightDir = normalize(vec3(1.0f, 1.0f, -1.0f));
    
    // Calculate diffuse lighting
    float diffuseIntensity = max(dot(normal, lightDir), 0.0f);

    // Use material index to select the correct texture
    outColor = texture(textures[pushConstants.materialIndex], fragTexcoord);

    outColor.rgb *= (ambientColor + diffuseColor*diffuseIntensity); // Apply diffuse lighting
    outColor.a = 1.0f; // Set alpha to 1.0 for opaque
}