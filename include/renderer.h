#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "base_camera.h"

class Renderer {
protected:
    std::vector<glm::vec3> mVertexPositions;
    std::shared_ptr<BaseCamera> mCamera;

    int mWindowWidth{0};
    int mWindowHeight{0};

public:
    Renderer(const std::vector<glm::vec3> &vertexPositions, std::shared_ptr<BaseCamera> camera)
        : mVertexPositions{vertexPositions}, mCamera{camera} {}
    virtual ~Renderer() {}

    virtual void initialize() = 0;
    virtual void destroy() = 0;
    virtual void draw() = 0;

    virtual void setWindowDimensions(int width, int height);

protected:
    static GLuint createShader(std::filesystem::path path, GLenum type);
};
