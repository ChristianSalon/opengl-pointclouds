#pragma once

#include <queue>
#include <unordered_map>

#include "path_utils.h"
#include "renderer.h"

class GlPointsRenderer : public Renderer {
public:
    const std::filesystem::path VS_PATH = getExecutableDirectory() / "shaders" / "gl_points.vert ";
    const std::filesystem::path FS_PATH = getExecutableDirectory() / "shaders" / "gl_points.frag ";

    struct OctreeNodeInfo {
        size_t pointCount;
        size_t offset;
    };

protected:
    std::unordered_map<size_t, OctreeNodeInfo> mOctreeInfo;
    std::vector<glm::vec4> mVertexPositions;
    std::vector<glm::u8vec4> mVertexColors;

    GLuint mProgram;
    GLuint mVs, mFs;
    GLuint mVao, mPositionVbo, mColorVbo;

    GLint mViewLocation;
    GLint mProjLocation;
    GLint mUseDefaultColorLocation;
    GLint mDefaultColorLocation;
    GLint mPointSizeLocation;

public:
    GlPointsRenderer(std::shared_ptr<PerspectiveCamera> camera, std::shared_ptr<Params> params) : Renderer{camera, params} {}
    virtual ~GlPointsRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual size_t draw() override;

protected:
    void processOctreeVertices();
};
