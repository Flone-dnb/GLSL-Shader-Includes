#glsl{
layout(binding = 0) uniform SomeData {
    uint iTest;
};
}

#hlsl ConstantBuffer<uint> someData : register(b0);

#glsl layout(binding = ?) uniform FrameData {
#hlsl struct FrameData{
    mat4 worldMatrix;
#glsl } frameData;
#hlsl }; ConstantBuffer<FrameData> frameData : register(b?);