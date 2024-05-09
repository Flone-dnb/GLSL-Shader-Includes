#glsl layout(binding = 0) uniform #hlsl struct #both ComputeInfo {
    uint iThreadGroupCountX;
} #glsl computeInfo; #hlsl ; ConstantBuffer<ComputeInfo> computeInfo : register(b0, space5);