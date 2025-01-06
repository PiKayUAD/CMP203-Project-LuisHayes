#include "Structs.hlsli"
#include "Lighting.hlsli"

ConstantBuffer<Constants> PushData : register(b0, space0);
StructuredBuffer<InstanceData> Instances : register(t0, space0);
StructuredBuffer<Light> Lights : register(t1, space0);

cbuffer FRAMEDATA : register(b1, space0)
{
    Frame FrameData;
}

SamplerState g_sampler : register(s0);

float4 main(VertexOutput input) : SV_Target0
{
    Texture2D texture = ResourceDescriptorHeap[Instances[PushData.instanceNo].textureIDX];

    return saturate(sqrt(texture.Sample(g_sampler, input.uv) * texture.Sample(g_sampler, input.uv) * Instances[PushData.instanceNo].diffuseColour * Instances[PushData.instanceNo].diffuseColour + input.colour * input.colour));
}