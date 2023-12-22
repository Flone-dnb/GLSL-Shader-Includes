layout(binding = ?) uniform FrameData {
    mat4 worldMatrix;
} frameData;

layout(binding = 1) uniform SomeData {
    uint iTest;
} someData;

layout(binding = ?) uniform NewData {
    mat4 viewMatrix;
} newData;