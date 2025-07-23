
struct FragmentInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct PushConstants
{
    float4x4 mvp;   // not used here, but in vertex shader
    uint64_t vertexBufferAddress;
    float3x3 normalMatrix;  // not used here, but in vertex shader
    uint32_t materialIndex;
    float padding[4];
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

[[vk::binding(0,0)]] Sampler2D textures[];

[shader("pixel")]
float4 main(FragmentInput input) : SV_Target
{
    float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
    float3 diffuseColor = float3(0.8f, 0.8f, 0.8f);

    float3 normal = normalize(input.normal);

    // Basic lighting direction (can be moved to constant buffer)
    float3 lightDir = normalize(float3(1.0f, 1.0f, 1.0f));
    
    // Calculate diffuse lighting
    float diffuseIntensity = max(dot(normal, lightDir), 0.0f);

    // Use material index to select the correct texture
    float4 outColor = textures[NonUniformResourceIndex(pushConstants.materialIndex)].Sample(input.texCoord);

    outColor.rgb *= (ambientColor + diffuseColor * diffuseIntensity); // Apply diffuse lighting
    outColor.a = 1.0f; // Set alpha to 1.0 for opaque
    
    return outColor;
}