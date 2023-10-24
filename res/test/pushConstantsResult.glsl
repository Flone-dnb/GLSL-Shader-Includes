#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Indices
{
    uint baseIndex;
    uint someIndex1;
    uint someIndex2;
    uint additionalIndex1;
    uint additionalIndex2;
uint additionalIndex3;
} indices;




void foo(){
    // some code here
}