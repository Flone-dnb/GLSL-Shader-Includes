
ConstantBuffer<uint> someData : register(b0);

struct FrameData{
    float4x4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b1);