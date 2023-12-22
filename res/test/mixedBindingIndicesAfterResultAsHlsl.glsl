struct FrameData{
    mat4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b1);


#hlsl ConstantBuffer<uint> someData : register(b0);