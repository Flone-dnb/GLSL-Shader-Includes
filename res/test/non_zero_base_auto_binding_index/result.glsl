layout(binding = 100) uniform FrameData {
    mat4 worldMatrix;
} frameData;

layout(binding = 1) uniform SomeData {
    uint iTest;
} someData;

layout(binding = 101) uniform NewData {
    mat4 viewMatrix;
} newData;