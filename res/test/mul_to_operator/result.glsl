void foo() {
    vec4 result1 = (matrix1 * vec4(somevec4.xyz, 1.0F));
    vec3 result2 = (matrix2 * somevec4).xyz;
}