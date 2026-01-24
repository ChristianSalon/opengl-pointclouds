#version 460

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec4 outColor;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 color;
uniform float pointSize = 1.0f;

void main() {
    vec4 vertex = vec4(inPos, 1.0f);       

    gl_Position = proj * view * vertex;
    gl_PointSize = pointSize;
    outColor = vec4(color, 1.0f);
}
