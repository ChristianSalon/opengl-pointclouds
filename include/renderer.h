#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "base_camera.h"

class Renderer {
public:
    struct Params {
        float pointSize = 1.0f;
        glm::vec3 pointColor = glm::vec3(0.0f, 0.0f, 1.0f);
        bool useDefaultColor = true;
        int holeFillingIterations = 2;
        float holeFillingInfluence = 0.01f;
        bool enableHoleFilling = true;
        int edlLevels = 5;
        float shadingFactor = 1.5f;
        float shadingStrength = 1.0f;
        bool enableEdl = true;
    };

protected:
    std::shared_ptr<BaseCamera> mCamera;
    int mWindowWidth{0};
    int mWindowHeight{0};

    std::vector<glm::vec4> mVertexPositions;
    std::vector<glm::u8vec4> mVertexColors;

    std::shared_ptr <Params> mParams{nullptr};

public:
    Renderer(std::shared_ptr<BaseCamera> camera, std::shared_ptr<Params> params) : mCamera{camera}, mParams{params} {}
    virtual ~Renderer() {}

    virtual void initialize() = 0;
    virtual void destroy() = 0;
    virtual void draw() = 0;

    virtual void setWindowDimensions(int width, int height);

    virtual void setPointCloud(const std::vector<glm::vec4> &vertexPositions, const std::vector<glm::u8vec4> &vertexColors);
    virtual void setPointCloud(const std::vector<glm::vec4> &vertexPositions);

protected:
    static GLuint createShader(std::filesystem::path path, GLenum type);
};
