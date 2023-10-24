    #version 450
    #extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform FrameData {
    mat4 worldMatrix;
} frameData;

    struct Nested{
        uint test;
    }

void foo(){
    mat4 matrix = frameData.worldMatrix;
}