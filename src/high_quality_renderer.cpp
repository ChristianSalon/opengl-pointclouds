#include <glm/gtc/type_ptr.hpp>

#include "high_quality_renderer.h"

void HighQualityRenderer::initialize() {
    mDepthCs = createShader(DEPTH_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mColorCs = createShader(COLOR_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mResolveCs = createShader(RESOLVE_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mQuadVs = createShader(QUAD_VS_PATH, GL_VERTEX_SHADER);
    mQuadFs = createShader(QUAD_FS_PATH, GL_FRAGMENT_SHADER);

    mDepthProgram = glCreateProgram();
    mColorProgram = glCreateProgram();
    mResolveProgram = glCreateProgram();
    mQuadProgram = glCreateProgram();

    glAttachShader(mDepthProgram, mDepthCs);
    glAttachShader(mColorProgram, mColorCs);
    glAttachShader(mResolveProgram, mResolveCs);
    glAttachShader(mQuadProgram, mQuadVs);
    glAttachShader(mQuadProgram, mQuadFs);

    glLinkProgram(mDepthProgram);
    glLinkProgram(mColorProgram);
    glLinkProgram(mResolveProgram);
    glLinkProgram(mQuadProgram);

    glCreateVertexArrays(1, &mQuadVao);

    glCreateBuffers(1, &mPointsSsbo);
    glNamedBufferStorage(mPointsSsbo, mVertexPositions.size() * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);
    std::vector<glm::vec4> packed;
    for (const glm::vec3 &vertex : mVertexPositions) {
        packed.push_back(glm::vec4(vertex, 1.f));
    }
    glNamedBufferSubData(mPointsSsbo, 0, packed.size() * sizeof(glm::vec4), packed.data());

    createFramebuffer();
    createOutputTexture();
}

void HighQualityRenderer::destroy() {
    glDeleteBuffers(1, &mPointsSsbo);
    glDeleteBuffers(1, &mFramebufferSsbo);
    glDeleteTextures(1, &mOutputTexture);

    glDeleteProgram(mDepthProgram);
    glDeleteProgram(mColorProgram);
    glDeleteProgram(mResolveProgram);
    glDeleteProgram(mQuadProgram);
}

void HighQualityRenderer::draw() {
    // Clear screen
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear framebuffer
    uint64_t framebufferClear = 0;
    glClearNamedBufferData(mFramebufferSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &framebufferClear);

    // Clear depth buffer
    uint32_t depthBufferClear = 0xffffffff;
    glClearNamedBufferData(mDepthSsbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &depthBufferClear);

    // Clear fallback buffer
    uint64_t fallbackClear = 0;
    glClearNamedBufferData(mFallbackSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &fallbackClear);

    // Depth pass
    glm::mat4 mvp = mCamera->projectionMatrix() * mCamera->viewMatrix();

    glUseProgram(mDepthProgram);
    glProgramUniformMatrix4fv(mDepthProgram, glGetUniformLocation(mDepthProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mDepthProgram, glGetUniformLocation(mDepthProgram, "framebufferSize"), mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mDepthSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Color pass
    glUseProgram(mColorProgram);
    glProgramUniformMatrix4fv(mColorProgram, glGetUniformLocation(mColorProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mColorProgram, glGetUniformLocation(mColorProgram, "framebufferSize"), mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFramebufferSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mDepthSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mFallbackSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Resolve pass
    glUseProgram(mResolveProgram);
    glProgramUniform2i(mResolveProgram, glGetUniformLocation(mResolveProgram, "framebufferSize"), mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFramebufferSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFallbackSsbo);
    glBindImageTexture(0, mOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Render fullscreen quad
    glDisable(GL_DEPTH_TEST);
    glUseProgram(mQuadProgram);

    glBindVertexArray(mQuadVao);
    glBindTextureUnit(0, mOutputTexture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void HighQualityRenderer::createFramebuffer() {
    size_t count = mWindowWidth * mWindowHeight;

    // Framebuffer
    glCreateBuffers(1, &mFramebufferSsbo);
    glNamedBufferStorage(mFramebufferSsbo, count * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Depth buffer
    glCreateBuffers(1, &mDepthSsbo);
    glNamedBufferStorage(mDepthSsbo, count * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Fallback buffer
    glCreateBuffers(1, &mFallbackSsbo);
    glNamedBufferStorage(mFallbackSsbo, count * 2 * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void HighQualityRenderer::createOutputTexture() {
    glCreateTextures(GL_TEXTURE_2D, 1, &mOutputTexture);
    glTextureStorage2D(mOutputTexture, 1, GL_RGBA8, mWindowWidth, mWindowHeight);

    glTextureParameteri(mOutputTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(mOutputTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void HighQualityRenderer::setWindowDimensions(int width, int height) {
    Renderer::setWindowDimensions(width, height);

    glFinish();

    glDeleteBuffers(1, &mFramebufferSsbo);
    glDeleteBuffers(1, &mDepthSsbo);
    glDeleteBuffers(1, &mFallbackSsbo);
    glDeleteTextures(1, &mOutputTexture);

    createFramebuffer();
    createOutputTexture();
}
