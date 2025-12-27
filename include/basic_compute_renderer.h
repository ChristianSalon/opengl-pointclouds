#pragma once

#include <glm/vec4.hpp>

#include "path_utils.h"
#include "renderer.h"

class BasicComputeRenderer : public Renderer {
public:
    const std::filesystem::path RENDER_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_render.comp";
    const std::filesystem::path RESOLVE_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_resolve.comp";
    const std::filesystem::path QUAD_VS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_quad.vert ";
    const std::filesystem::path QUAD_FS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_quad.frag ";

protected:
    GLuint mRenderProgram;
    GLuint mResolveProgram;
    GLuint mQuadProgram;

    GLuint mRenderCs, mResolveCs;
    GLuint mQuadVs, mQuadFs;

    GLuint mPointsSsbo;
    GLuint mFramebufferSsbo;
    GLuint mOutputTexture;

    GLuint mQuadVao;

public:
    BasicComputeRenderer(const std::vector<glm::vec3> &vertexPositions, std::shared_ptr<BaseCamera> camera)
        : Renderer{vertexPositions, camera} {}
    virtual ~BasicComputeRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual void draw() override;

    virtual void setWindowDimensions(int width, int height) override;

protected:
    void createFramebuffer();
    void createOutputTexture();
};
