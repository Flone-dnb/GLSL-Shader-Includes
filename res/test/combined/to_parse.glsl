#glsl {
#version 450
#extension GL_ARB_separate_shader_objects : enable
}

#glsl layout(binding = ?) uniform FrameData {
#hlsl struct FrameData {
        mat4 worldMatrix;
#glsl } frameData;
#hlsl }; ConstantBuffer<FrameData> frameData : register(b?);

#hlsl ConstantBuffer<uint> someData : register(b?, space5);
#hlsl ConstantBuffer<uint> someData1 : register(b?, space5);
#hlsl ConstantBuffer<uint> someData2 : register(b?, space0);

#glsl {
    struct Nested {
        uint test;
    }
}

shared uint iGroupSharedVariable;

void foo() {
    mat4 matrix = frameData.worldMatrix;
    vec3 someVec = vec3(0.0F, 0.0F, 0.0F);
    bar(vec3(myvec2, 0.0));
}