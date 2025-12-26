#version 450

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec4 outColor;

uniform mat4 view;
uniform mat4 proj;

void main() {
    vec4 vertex = vec4(inPos, 1.f);       

    gl_Position = proj * view * vertex;
    outColor = vec4(0.f, 0.f, 1.f, 1.f);
}
