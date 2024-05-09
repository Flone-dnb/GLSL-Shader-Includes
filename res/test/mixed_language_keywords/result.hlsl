struct ComputeInfo {
    uint iThreadGroupCountX;
} ; ConstantBuffer<ComputeInfo> computeInfo : register(b0, space5);