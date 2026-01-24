#version 460

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

uniform mat4 view;
uniform mat4 proj;
uniform bool useDefaultColor;
uniform vec4 defaultColor;
uniform float pointSize = 1.0f;

void main() {
    gl_Position = proj * view * inPosition;
    gl_PointSize = pointSize;
    outColor = useDefaultColor ? defaultColor : inColor;
}
