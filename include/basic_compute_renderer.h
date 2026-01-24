#pragma once

#include <glm/vec4.hpp>

#include "path_utils.h"
#include "renderer.h"

class BasicComputeRenderer : public Renderer {
public:
    const std::filesystem::path RENDER_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_render.comp";
    const std::filesystem::path HOLE_FILLING_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_hole_filling.comp";
    const std::filesystem::path EDL_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_edl.comp";
    const std::filesystem::path RESOLVE_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_resolve.comp";
    const std::filesystem::path QUAD_VS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_quad.vert ";
    const std::filesystem::path QUAD_FS_PATH = getExecutableDirectory() / "shaders" / "basic_compute_quad.frag ";

protected:
    GLuint mRenderProgram;
    GLuint mHoleFillingProgram;
    GLuint mEdlProgram;
    GLuint mResolveProgram;
    GLuint mQuadProgram;

    GLuint mRenderCs, mHoleFillingCs, mEdlCs, mResolveCs;
    GLuint mQuadVs, mQuadFs;

    GLint mRenderProgramUseDefaultColorLocation, mRenderProgramDefaultColorLocation, mRenderProgramMvpLocation, mRenderProgramFramebufferSizeLocation;
    GLint mHoleFillingProgramFramebufferSizeLocation, mHoleFillingProgramIterationLocation, mHoleFillingProgramInfluenceLocation;
    GLint mEdlProgramFramebufferSizeLocation, mEdlProgramLevelLocation, mEdlProgramShadingFactorLocation;
    GLint mResolveProgramFramebufferSizeLocation, mResolveProgramUseEdlLocation, mResolveProgramEdlShadingStrengthLocation;

    GLuint mPointsSsbo, mColorSsbo;
    GLuint mFirstFramebufferSsbo, mSecondFramebufferSsbo, mEmptyMaskSsbo;
    GLuint mFirstEdlSsbo, mSecondEdlSsbo;
    GLuint mOutputTexture;

    GLuint mQuadVao;


public:
    BasicComputeRenderer(std::shared_ptr<BaseCamera> camera, std::shared_ptr<Params> params) : Renderer{camera, params} {}
    virtual ~BasicComputeRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual void draw() override;

    virtual void setWindowDimensions(int width, int height) override;

protected:
    void createFramebuffer();
    void createOutputTexture();
};
