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
    mUseDefaultColorLocation = glGetUniformLocation(mProgram, "useDefaultColor");
    mDefaultColorLocation = glGetUniformLocation(mProgram, "defaultColor");
    mPointSizeLocation = glGetUniformLocation(mProgram, "pointSize");

    glCreateVertexArrays(1, &mVao);
    glCreateBuffers(1, &mPositionVbo);
    glCreateBuffers(1, &mColorVbo);

    glNamedBufferStorage(mPositionVbo, mVertexPositions.size() * sizeof(glm::vec4), mVertexPositions.data(), 0);
    glNamedBufferStorage(mColorVbo, mVertexColors.size() * sizeof(glm::u8vec4), mVertexColors.data(), 0);

    glEnableVertexArrayAttrib(mVao, 0);
    glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mVao, 0, 0);
    glVertexArrayVertexBuffer(mVao, 0, mPositionVbo, 0, sizeof(glm::vec4));

    glEnableVertexArrayAttrib(mVao, 1);
    glVertexArrayAttribFormat(mVao, 1, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0);
    glVertexArrayAttribBinding(mVao, 1, 1);
    glVertexArrayVertexBuffer(mVao, 1, mColorVbo, 0, sizeof(glm::u8vec4));
}

void GlPointsRenderer::destroy() {
    glDeleteProgram(mProgram);

    glDeleteShader(mVs);
    glDeleteShader(mFs);

    glDeleteVertexArrays(1, &mVao);

    glDeleteBuffers(1, &mPositionVbo);
    glDeleteBuffers(1, &mColorVbo);
}

void GlPointsRenderer::draw() {
    // OpenGL clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // OpenGl Draw
    glUseProgram(mProgram);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glm::mat4 viewMatrix = mCamera->viewMatrix();
    glm::mat4 projectionMatrix = mCamera->projectionMatrix();
    glm::vec4 defaultColor = glm::vec4(mParams->pointColor, 1.0f);

    glProgramUniformMatrix4fv(mProgram, mViewLocation, 1, GL_FALSE, (float *)&viewMatrix);
    glProgramUniformMatrix4fv(mProgram, mProjLocation, 1, GL_FALSE, (float *)&projectionMatrix);
    glProgramUniform1i(mProgram, mUseDefaultColorLocation, mParams->useDefaultColor);
    glProgramUniform4fv(mProgram, mDefaultColorLocation, 1, (float *)glm::value_ptr(defaultColor));
    glUniform1f(glGetUniformLocation(mProgram, "pointSize"), mParams->pointSize);

    glBindVertexArray(mVao);
    glDrawArrays(GL_POINTS, 0, mVertexPositions.size());
}
