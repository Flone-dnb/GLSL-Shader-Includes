
ConstantBuffer<uint> someData : register(b0);

struct FrameData{
    mat4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b1);