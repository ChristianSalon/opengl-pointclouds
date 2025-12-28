#pragma once

#include <glm/vec4.hpp>

#include "path_utils.h"
#include "renderer.h"

class HighQualityRenderer : public Renderer {
public:
    const std::filesystem::path DEPTH_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_depth.comp";
    const std::filesystem::path COLOR_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_color.comp";
    const std::filesystem::path RESOLVE_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_resolve.comp";
    const std::filesystem::path QUAD_VS_PATH = getExecutableDirectory() / "shaders" / "high_quality_quad.vert ";
    const std::filesystem::path QUAD_FS_PATH = getExecutableDirectory() / "shaders" / "high_quality_quad.frag ";

protected:
    GLuint mDepthProgram;
    GLuint mColorProgram;
    GLuint mResolveProgram;
    GLuint mQuadProgram;

    GLuint mDepthCs, mColorCs, mResolveCs;
    GLuint mQuadVs, mQuadFs;

    GLuint mPointsSsbo;
    GLuint mDepthSsbo;
    GLuint mFramebufferSsbo;
    GLuint mFallbackSsbo;
    GLuint mOutputTexture;

    GLuint mQuadVao;

public:
    HighQualityRenderer(const std::vector<glm::vec3> &vertexPositions, std::shared_ptr<BaseCamera> camera)
        : Renderer{vertexPositions, camera} {}
    virtual ~HighQualityRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual void draw() override;

    virtual void setWindowDimensions(int width, int height) override;

protected:
    void createFramebuffer();
    void createOutputTexture();
};
