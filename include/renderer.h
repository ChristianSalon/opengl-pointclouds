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

    std::vector<glm::vec3> mVertexPositions;

    Params mParams{};

public:
    Renderer(std::shared_ptr<BaseCamera> camera) : mCamera{camera} {}
    virtual ~Renderer() {}

    virtual void initialize() = 0;
    virtual void destroy() = 0;
    virtual void draw() = 0;

    virtual void setWindowDimensions(int width, int height);
    virtual void setPointCloud(const std::vector<glm::vec3> &vertexPositions);

    virtual Params &getParams() { return mParams; }

protected:
    static GLuint createShader(std::filesystem::path path, GLenum type);
};
