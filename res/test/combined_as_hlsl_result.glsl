
struct FrameData{
    float4x4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b0, space5);

void foo(){
    mat4 matrix = frameData.worldMatrix;
}