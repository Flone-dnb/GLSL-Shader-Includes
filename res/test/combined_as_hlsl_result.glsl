

SamplerState textureSampler : register(s0, space5);

struct FrameData{
    float4x4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b0);

ConstantBuffer<uint> someData : register(b0, space5);
ConstantBuffer<uint> someData1 : register(b1, space5);
ConstantBuffer<uint> someData2 : register(b1, space0);


groupshared uint iGroupSharedVariable;

void foo(){
    float4x4 matrix = frameData.worldMatrix;
    float3 someVec = float3(0.0F, 0.0F, 0.0F);
}