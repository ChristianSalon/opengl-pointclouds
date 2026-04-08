#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vColor;

uniform mat4 view;
uniform mat4 proj;
uniform bool useDefaultColor;
uniform vec4 defaultColor;

void main() {
    gl_Position = proj * view * vec4(inPosition, 1.0);

    vPosition = inPosition;
    vNormal = inNormal;
    vColor = useDefaultColor ? defaultColor : inColor;
}
