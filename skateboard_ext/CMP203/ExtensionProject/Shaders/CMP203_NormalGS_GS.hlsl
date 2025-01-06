#include "Structs.hlsli"

struct PixelInput
{
    float4 position : SV_Position; // Careful here, it's different!!
    float3 normal : NORMAL;
};

ConstantBuffer<Constants> PushData : register(b0, space0);
StructuredBuffer<InstanceData> Instances : register(t0, space0);

cbuffer FRAMEDATA : register(b1, space0)
{
    Frame FrameData;
}

[maxvertexcount(6)]
void main(triangle VertexOutput input[3], inout LineStream<PixelInput> lStream)
{
    PixelInput output;
    for (int i = 0; i < 3; i++)
    {
        float3 P = input[i].position.xyz;
        float3 N = input[i].normal.xyz;
        float3 P2 = P + (N / 2);
        
        output.position = mul(Instances[PushData.instanceNo].World, float4(P, 1.0f));
        output.position = mul(FrameData.View, output.position);
        output.position = mul(FrameData.Projection, output.position);
        output.normal = mul((float3x3) Instances[PushData.instanceNo].World, N);
        output.normal = normalize(output.normal);
        
        lStream.Append(output);
        output.position = mul(Instances[PushData.instanceNo].World, float4(P2, 1.0f));
        output.position = mul(FrameData.View, output.position);
        output.position = mul(FrameData.Projection, output.position);
        lStream.Append(output);
        lStream.RestartStrip();
    }
}
