#version 460

out vec2 vTexCoord;

vec2 positions[] = {
    vec2(-1.f, -1.f), 
    vec2(1.f, -1.f), 
    vec2(-1.f, 1.f),
    vec2(1.f, 1.f), 
    vec2(-1.f, 1.f), 
    vec2(1.f, -1.f)
};

void main() {
    vTexCoord = (positions[gl_VertexID].xy + 1.f) * 0.5f;
    gl_Position = vec4(positions[gl_VertexID], 0.f, 1.f);
}
