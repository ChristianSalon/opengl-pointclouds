#include "gl_points_renderer.h"

void GlPointsRenderer::initialize() {
    mVs = createShader(VS_PATH, GL_VERTEX_SHADER);
    mFs = createShader(FS_PATH, GL_FRAGMENT_SHADER);

    mProgram = glCreateProgram();

    glAttachShader(mProgram, mVs);
    glAttachShader(mProgram, mFs);

    glLinkProgram(mProgram);

    mViewLocation = glGetUniformLocation(mProgram, "view");
    mProjLocation = glGetUniformLocation(mProgram, "proj");

    glCreateVertexArrays(1, &mVao);
    glCreateBuffers(1, &mVbo);

    glNamedBufferStorage(mVbo, mVertexPositions.size() * sizeof(glm::vec3), mVertexPositions.data(), 0);

    glEnableVertexArrayAttrib(mVao, 0);
    glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mVao, 0, 0);

    glVertexArrayVertexBuffer(mVao, 0, mVbo, 0, sizeof(glm::vec3));
}

void GlPointsRenderer::destroy() {
    glDeleteProgram(mProgram);
    glDeleteShader(mVs);
    glDeleteShader(mFs);
}

void GlPointsRenderer::draw() {
    // OpenGL clear
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // OpenGl Draw
    glUseProgram(mProgram);

    glm::mat4 viewMatrix = mCamera->viewMatrix();
    glm::mat4 projectionMatrix = mCamera->projectionMatrix();
    glProgramUniformMatrix4fv(mProgram, mViewLocation, 1, GL_FALSE, (float *) &viewMatrix);
    glProgramUniformMatrix4fv(mProgram, mProjLocation, 1, GL_FALSE, (float *) &projectionMatrix);

    glBindVertexArray(mVao);
    glDrawArrays(GL_POINTS, 0, mVertexPositions.size());
}
