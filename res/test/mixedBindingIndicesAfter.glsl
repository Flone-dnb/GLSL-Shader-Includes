#glsl layout(binding = ?) uniform FrameData {
#hlsl struct FrameData{
    mat4 worldMatrix;
#glsl } frameData;
#hlsl }; ConstantBuffer<FrameData> frameData : register(b?);

#glsl{
layout(binding = 1) uniform SomeData {
    uint iTest;
};
}

#hlsl ConstantBuffer<uint> someData : register(b1);