#pragma once

#include "path_utils.h"
#include "renderer.h"

class GlPointsRenderer : public Renderer {
public:
    const std::filesystem::path VS_PATH = getExecutableDirectory() / "shaders" / "gl_points.vert ";
    const std::filesystem::path FS_PATH = getExecutableDirectory() / "shaders" / "gl_points.frag ";

protected:
    GLuint mProgram;
    GLuint mVs, mFs;
    GLuint mVao, mVbo;

    GLint mViewLocation;
    GLint mProjLocation;
    GLint mColorLocation;
    GLint mPointSizeLocation;

public:
    GlPointsRenderer(std::shared_ptr<BaseCamera> camera) : Renderer{camera} {}
    virtual ~GlPointsRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual void draw() override;
};
