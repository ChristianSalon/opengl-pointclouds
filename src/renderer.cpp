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

Renderer::BoundingBox Renderer::setPointCloud(std::shared_ptr<OctreeBuilder::OctreeNode> octree) {
    BoundingBox bbox;
    if (octree) {
        bbox.center = octree->boundingCube.center;
        bbox.radius = octree->boundingCube.halfSize;
    }

    mOctree = octree;

    glFinish();

    destroy();
    initialize();

    return bbox;
}
