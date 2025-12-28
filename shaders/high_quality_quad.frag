#version 460

in vec2 vTexCoord;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uTexture;

void main() {
    fColor = texture(uTexture, vTexCoord);
}
