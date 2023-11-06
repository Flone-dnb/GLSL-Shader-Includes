
struct FrameData{
    float4x4 worldMatrix;
}; ConstantBuffer<FrameData> frameData : register(b0, space5);


void foo(){
    float4x4 matrix = frameData.worldMatrix;
    float3 someVec = float3(0.0F, 0.0F, 0.0F);
}