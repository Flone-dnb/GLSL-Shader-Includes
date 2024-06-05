void foo() {
    float4 result1 = mul(matrix1, float4(somevec4.xyz, 1.0F));
    float3 result2 = mul(matrix2, somevec4).xyz;
    
    //vec4 result1 = mul(matrix1, vec4(somevec4.xyz, 1.0F));
    notmul(matrix1, float4(somevec4.xyz, 1.0F));
    
    /**
     * mul not here
     */
}