#define MESH_SHADING 0

struct Vertex
{
    float3 pos : POSITION0;
    float2 texCoord;
    float3 normal;
    float4 tangent;
};

struct VertexBuffer
{
    Vertex vertices[];
}

struct PushConstants
{
    float32_t4x4 mvp;
    VertexBuffer* vertexBuffer;
    float32_t3x3 normalMatrix;
    uint32_t albedoIndex; // not used here
    uint32_t normalIndex;
    uint32_t metallicIndex;
    uint32_t emissiveIndex;
    float padding;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

[shader("vertex")]
VertexOutput main(uint vertexIndex : SV_VertexID) 
{
    VertexOutput output;
    Vertex v = pushConstants.vertexBuffer.vertices[vertexIndex];

    output.position = mul(pushConstants.mvp, float4(v.pos, 1.0));
    output.texCoord = v.texCoord;
    output.normal = mul(v.normal, pushConstants.normalMatrix, );
    output.tangent = mul(v.tangent.xyz, pushConstants.normalMatrix, );
    return output;
}