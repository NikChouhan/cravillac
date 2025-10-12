#define MESH_ENABLED 0
#define BDA 0
struct VertexOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
};

// BDA path

//struct Vertex
//{
//    float3 pos;
//    float2 texCoord;
//    float3 normal;
//    float4 tangent;
//};

//struct VertexBuffer
//{
//    Vertex vertices[];
//};

//struct PushConstants
//{
//    float32_t4x4 mvp;
//    VertexBuffer* vertexBuffer;
//    float32_t3x3 normalMatrix;
//    uint32_t albedoIndex;
//    uint32_t normalIndex;
//    uint32_t metallicIndex;
//    uint32_t emissiveIndex;
//    float    padding;
//};

//[[vk::push_constant]]
//ConstantBuffer<PushConstants> pushConstants;

//[shader("vertex")]
//VertexOutput main(uint vertexIndex : SV_VertexID)
//{
//    VertexOutput output;
//    Vertex v = pushConstants.vertexBuffer.vertices[vertexIndex];

//    output.position = mul(pushConstants.mvp, float4(v.pos, 1.0));
//    output.texCoord = v.texCoord;
//    output.normal   = normalize(mul(pushConstants.normalMatrix, v.normal));
//    output.tangent  = normalize(mul(pushConstants.normalMatrix, v.tangent.xyz));

//    return output;
//}
// vertex attrib path

struct VertexInput
{
    float3 pos      : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
};

struct PushConstants
{
    float32_t4x4 mvp;
    float32_t3x3 normalMatrix;
    uint32_t albedoIndex;
    uint32_t normalIndex;
    uint32_t metallicIndex;
    uint32_t emissiveIndex;
    float    padding;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    output.position = mul(pushConstants.mvp, float4(input.pos, 1.0));
    output.texCoord = input.texCoord;
    output.normal   = normalize(mul(pushConstants.normalMatrix, input.normal));
    output.tangent  = normalize(mul(pushConstants.normalMatrix, input.tangent.xyz));
    
    return output;
}