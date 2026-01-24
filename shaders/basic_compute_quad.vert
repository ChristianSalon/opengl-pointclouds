#version 460

out vec2 vTexCoord;

vec2 positions[] = {
    vec2(-1.0f, -1.0f), 
    vec2(1.0f, -1.0f), 
    vec2(-1.0f, 1.0f),
    vec2(1.0f, 1.0f), 
    vec2(-1.0f, 1.0f), 
    vec2(1.0f, -1.0f)
};

void main() {
    vTexCoord = (positions[gl_VertexID].xy + 1.0f) * 0.5f;
    gl_Position = vec4(positions[gl_VertexID], 0.0f, 1.0f);
}
