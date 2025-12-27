#include <limits>

#include <glm/gtc/type_ptr.hpp>

#include "basic_compute_renderer.h"

void BasicComputeRenderer::initialize() {
    mRenderCs = createShader(RENDER_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mResolveCs = createShader(RESOLVE_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mQuadVs = createShader(QUAD_VS_PATH, GL_VERTEX_SHADER);
    mQuadFs = createShader(QUAD_FS_PATH, GL_FRAGMENT_SHADER);

    mRenderProgram = glCreateProgram();
    mResolveProgram = glCreateProgram();
    mQuadProgram = glCreateProgram();

    glAttachShader(mRenderProgram, mRenderCs);
    glAttachShader(mResolveProgram, mResolveCs);
    glAttachShader(mQuadProgram, mQuadVs);
    glAttachShader(mQuadProgram, mQuadFs);

    glLinkProgram(mRenderProgram);
    glLinkProgram(mResolveProgram);
    glLinkProgram(mQuadProgram);

    glCreateBuffers(1, &mPointsSsbo);
    glNamedBufferStorage(mPointsSsbo, mVertexPositions.size() * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);

    std::vector<glm::vec4> packed;
    for (const glm::vec3 &vertex : mVertexPositions) {
        packed.push_back(glm::vec4(vertex, 1.f));
    }

    glNamedBufferSubData(mPointsSsbo, 0, packed.size() * sizeof(glm::vec4), packed.data());

    glCreateVertexArrays(1, &mQuadVao);

    createFramebuffer();
    createOutputTexture();
}

void BasicComputeRenderer::destroy() {
    glDeleteBuffers(1, &mPointsSsbo);
    glDeleteBuffers(1, &mFramebufferSsbo);
    glDeleteTextures(1, &mOutputTexture);

    glDeleteProgram(mRenderProgram);
    glDeleteProgram(mResolveProgram);
    glDeleteProgram(mQuadProgram);
}

void BasicComputeRenderer::draw() {
    // Clear screen
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear framebuffer
    uint64_t framebufferClear = 0xffffffffff1a1a1a;
    glClearNamedBufferData(mFramebufferSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &framebufferClear);

    // Render pass
    glm::mat4 mvp = mCamera->projectionMatrix() * mCamera->viewMatrix();

    glUseProgram(mRenderProgram);
    glProgramUniformMatrix4fv(mRenderProgram, glGetUniformLocation(mRenderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mRenderProgram, glGetUniformLocation(mRenderProgram, "framebufferSize"), mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFramebufferSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Resolve pass
    glUseProgram(mResolveProgram);
    glProgramUniform2i(mResolveProgram, glGetUniformLocation(mResolveProgram, "framebufferSize"), mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFramebufferSsbo);
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

void BasicComputeRenderer::createFramebuffer() {
    size_t count = mWindowWidth * mWindowHeight;

    glCreateBuffers(1, &mFramebufferSsbo);
    glNamedBufferStorage(mFramebufferSsbo, count * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void BasicComputeRenderer::createOutputTexture() {
    glCreateTextures(GL_TEXTURE_2D, 1, &mOutputTexture);
    glTextureStorage2D(mOutputTexture, 1, GL_RGBA8, mWindowWidth, mWindowHeight);

    glTextureParameteri(mOutputTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(mOutputTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void BasicComputeRenderer::setWindowDimensions(int width, int height) {
    Renderer::setWindowDimensions(width, height);

    glFinish();

    glDeleteBuffers(1, &mFramebufferSsbo);
    glDeleteTextures(1, &mOutputTexture);

    createFramebuffer();
    createOutputTexture();
}
