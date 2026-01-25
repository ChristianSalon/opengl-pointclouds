#include <fstream>
#include <sstream>
#include <iostream>

#include "renderer.h"

GLuint Renderer::createShader(std::filesystem::path path, GLenum type) {
    std::ifstream file{path, std::istream::in | std::ios::binary};
    if (file.fail()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    std::string code = ss.str();
    if (code.empty()) {
        throw std::runtime_error("Shader " + path.string() + " is empty");
    }

    const char *shaderSource = code.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    return shader;
}

void Renderer::setWindowDimensions(int width, int height) {
    mWindowWidth = width;
    mWindowHeight = height;
}

void Renderer::setPointCloud(const std::vector<glm::vec4> &&vertexPositions, const std::vector<glm::u8vec4> &&vertexColors) {
    OctreeBuilder builder;
    mOctree = builder.build(std::move(vertexPositions), std::move(vertexColors));

    glFinish();

    destroy();
    initialize();
}

void Renderer::setPointCloud(const std::vector<glm::vec4> &&vertexPositions) {
    std::vector<glm::u8vec4> vertexColors{};
    setPointCloud(std::move(vertexPositions), std::move(vertexColors));
}
