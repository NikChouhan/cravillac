
struct FragmentInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

struct PushConstants
{
    float4x4 mvp;   // not used here, but in vertex shader
    uint64_t vertexBufferAddress;
    float3x3 normalMatrix;
    uint32_t albedoIndex;
    uint32_t normalIndex;
    uint32_t metallicIndex;
    uint32_t emissiveIndex;
    float padding;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

[[vk::binding(0,0)]] Sampler2D textures[];

[shader("pixel")]
float4 main(FragmentInput input) : SV_Target
{
    float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
    float3 diffuseColor = float3(0.8f, 0.8f, 0.8f);
    float3 lightDir = normalize(float3(1.0f, 1.0f, 1.0f));
    // the following shader works although it looks weird cuz the lightdir is in the world space even when the normals are in view space
    // pass its direction in the view space so that it doesnt change with the camera direction

    // float3 normal = textures[NonUniformResourceIndex(pushConstants.normalIndex)].Sample(input.texCoord).xyz;
    // normal = normal*2.f - 1.f;  // convert from the normal texture [0,1] space to ndc [-1,1]
    // normal = normalize(mul(normal, input.tangent));
    // float diffuseIntensity = max(dot(normal, lightDir), 0.0f);
                // OR
    float3 normal = normalize(input.normal);
    float diffuseIntensity = max(dot(normal, lightDir), 0.0f);


    // Use material index to select the correct texture
    float4 outColor = textures[NonUniformResourceIndex(pushConstants.albedoIndex)].Sample(input.texCoord);

    if(pushConstants.emissiveIndex > -1)
    {
        float3 emissive = textures[NonUniformResourceIndex(pushConstants.emissiveIndex)].Sample(input.texCoord).xyz;
        outColor.rgb *= (ambientColor + diffuseColor * diffuseIntensity + emissive);
    }
    outColor.rgb *= (ambientColor + diffuseColor * diffuseIntensity); // Apply diffuse lighting
    outColor.a = 1.0f; // Set alpha to 1.0 for opaque
    
    return outColor;
}