    #version 450
    #extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform AdditionalData1 {
uint test;
} additionalData1;


layout(binding = 1) uniform FrameData {
    mat4 worldMatrix;
} frameData;


    struct Nested{
        uint test;
    }

shared uint iGroupSharedVariable;

void foo(){
    mat4 matrix = frameData.worldMatrix;
    vec3 someVec = vec3(0.0F, 0.0F, 0.0F);
}