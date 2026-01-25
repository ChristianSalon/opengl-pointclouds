#pragma once

#include <queue>
#include <unordered_map>

#include <glm/vec4.hpp>

#include "path_utils.h"
#include "renderer.h"

class HighQualityRenderer : public Renderer {
public:
    const std::filesystem::path DEPTH_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_depth.comp";
    const std::filesystem::path COLOR_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_color.comp";
    const std::filesystem::path RESOLVE_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_resolve.comp";
    const std::filesystem::path HOLE_FILLING_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_hole_filling.comp";
    const std::filesystem::path EDL_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_edl.comp";
    const std::filesystem::path SECOND_RESOLVE_PASS_CS_PATH = getExecutableDirectory() / "shaders" / "high_quality_second_resolve.comp";
    const std::filesystem::path QUAD_VS_PATH = getExecutableDirectory() / "shaders" / "high_quality_quad.vert ";
    const std::filesystem::path QUAD_FS_PATH = getExecutableDirectory() / "shaders" / "high_quality_quad.frag ";

    struct OctreeNodeInfo {
        size_t pointCount;
        size_t offset;
    };

protected:
    std::unordered_map<size_t, OctreeNodeInfo> mOctreeInfo;
    std::vector<glm::vec4> mVertexPositions;
    std::vector<glm::u8vec4> mVertexColors;

    GLuint mDepthProgram;
    GLuint mColorProgram;
    GLuint mResolveProgram;
    GLuint mHoleFillingProgram;
    GLuint mEdlProgram;
    GLuint mSecondResolveProgram;
    GLuint mQuadProgram;

    GLuint mDepthCs, mColorCs, mResolveCs, mHoleFillingCs, mEdlCs, mSecondResolveCs;
    GLuint mQuadVs, mQuadFs;
    
    GLint mDepthProgramMvpLocation, mDepthProgramFramebufferSizeLocation, mDepthProgramVertexOffsetLoction,
        mDepthProgramVertexCountLoction;
    GLint mColorProgramUseDefaultColorLocation, mColorProgramDefaultColorLocation, mColorProgramMvpLocation,
        mColorProgramFramebufferSizeLocation, mColorProgramVertexOffsetLoction, mColorProgramVertexCountLoction;
    GLint mResolveProgramFramebufferSizeLocation;
    GLint mHoleFillingProgramFramebufferSizeLocation, mHoleFillingProgramIterationLocation, mHoleFillingProgramInfluenceLocation;
    GLint mEdlProgramFramebufferSizeLocation, mEdlProgramLevelLocation, mEdlProgramShadingFactorLocation;
    GLint mSecondResolveProgramFramebufferSizeLocation, mSecondResolveProgramUseEdlLocation, mSecondResolveProgramEdlShadingStrengthLocation;

    GLuint mPointsSsbo, mColorSsbo;
    GLuint mFirstDepthSsbo, mSecondDepthSsbo;
    GLuint mFramebufferSsbo;
    GLuint mFallbackSsbo;
    GLuint mFirstOutputTexture, mSecondOutputTexture;
    GLuint mEmptyMaskSsbo, mFirstEdlSsbo, mSecondEdlSsbo;

    GLuint mQuadVao;

public:
    HighQualityRenderer(std::shared_ptr<PerspectiveCamera> camera, std::shared_ptr<Params> params) : Renderer{camera, params} {}
    virtual ~HighQualityRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual size_t draw() override;

    virtual void setWindowDimensions(int width, int height) override;

protected:
    void processOctreeVertices();
    void createFramebuffer();
    void createOutputTexture();
};
