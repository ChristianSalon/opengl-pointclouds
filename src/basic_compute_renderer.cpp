#include <glm/gtc/type_ptr.hpp>

#include "basic_compute_renderer.h"

void BasicComputeRenderer::initialize() {
    mRenderCs = createShader(RENDER_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mHoleFillingCs = createShader(HOLE_FILLING_CS_PATH, GL_COMPUTE_SHADER);
    mEdlCs = createShader(EDL_CS_PATH, GL_COMPUTE_SHADER);
    mResolveCs = createShader(RESOLVE_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mQuadVs = createShader(QUAD_VS_PATH, GL_VERTEX_SHADER);
    mQuadFs = createShader(QUAD_FS_PATH, GL_FRAGMENT_SHADER);

    mRenderProgram = glCreateProgram();
    mHoleFillingProgram = glCreateProgram();
    mEdlProgram = glCreateProgram();
    mResolveProgram = glCreateProgram();
    mQuadProgram = glCreateProgram();

    glAttachShader(mRenderProgram, mRenderCs);
    glAttachShader(mHoleFillingProgram, mHoleFillingCs);
    glAttachShader(mEdlProgram, mEdlCs);
    glAttachShader(mResolveProgram, mResolveCs);
    glAttachShader(mQuadProgram, mQuadVs);
    glAttachShader(mQuadProgram, mQuadFs);

    glLinkProgram(mRenderProgram);
    glLinkProgram(mHoleFillingProgram);
    glLinkProgram(mEdlProgram);
    glLinkProgram(mResolveProgram);
    glLinkProgram(mQuadProgram);

    mRenderProgramColorLocation = glGetUniformLocation(mRenderProgram, "color");
    mRenderProgramMvpLocation = glGetUniformLocation(mRenderProgram, "mvp");
    mRenderProgramFramebufferSizeLocation = glGetUniformLocation(mRenderProgram, "framebufferSize");

    mHoleFillingProgramFramebufferSizeLocation = glGetUniformLocation(mHoleFillingProgram, "framebufferSize");
    mHoleFillingProgramIterationLocation = glGetUniformLocation(mHoleFillingProgram, "iteration");
    mHoleFillingProgramInfluenceLocation = glGetUniformLocation(mHoleFillingProgram, "influence");

    mEdlProgramFramebufferSizeLocation = glGetUniformLocation(mEdlProgram, "framebufferSize");
    mEdlProgramLevelLocation = glGetUniformLocation(mEdlProgram, "level");
    mEdlProgramShadingFactorLocation = glGetUniformLocation(mEdlProgram, "shadingFactor");
    
    mResolveProgramFramebufferSizeLocation = glGetUniformLocation(mResolveProgram, "framebufferSize");
    mResolveProgramEdlShadingStrengthLocation = glGetUniformLocation(mResolveProgram, "edlShadingStrength");
    mResolveProgramUseEdlLocation = glGetUniformLocation(mResolveProgram, "useEdl");

    glCreateBuffers(1, &mPointsSsbo);
    glNamedBufferStorage(mPointsSsbo, mVertexPositions.size() * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);

    std::vector<glm::vec4> packed;
    for (const glm::vec3 &vertex : mVertexPositions) {
        packed.push_back(glm::vec4(vertex, 1.0f));
    }

    glNamedBufferSubData(mPointsSsbo, 0, packed.size() * sizeof(glm::vec4), packed.data());

    glCreateVertexArrays(1, &mQuadVao);

    createFramebuffer();
    createOutputTexture();
}

void BasicComputeRenderer::destroy() {
    glDeleteBuffers(1, &mPointsSsbo);
    glDeleteBuffers(1, &mFirstFramebufferSsbo);
    glDeleteBuffers(1, &mSecondFramebufferSsbo);
    glDeleteBuffers(1, &mEmptyMaskSsbo);
    glDeleteBuffers(1, &mFirstEdlSsbo);
    glDeleteBuffers(1, &mSecondEdlSsbo);
    glDeleteTextures(1, &mOutputTexture);

    glDeleteProgram(mRenderProgram);
    glDeleteProgram(mHoleFillingProgram);
    glDeleteProgram(mEdlProgram);
    glDeleteProgram(mResolveProgram);
    glDeleteProgram(mQuadProgram);
}

void BasicComputeRenderer::draw() {
    // Clear screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear framebuffer
    uint64_t framebufferClear = 0xffffffffff000000;
    glClearNamedBufferData(mFirstFramebufferSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &framebufferClear);
    glClearNamedBufferData(mSecondFramebufferSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &framebufferClear);

    uint32_t errorMaskClear = 0;
    glClearNamedBufferData(mEmptyMaskSsbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &errorMaskClear);

    // Edl level info
    // depth, shading_term
    glm::vec2 edlLevelClear(0.0f, 0.0f);
    glClearNamedBufferData(mFirstEdlSsbo, GL_RG32F, GL_RG, GL_FLOAT, &edlLevelClear);
    glClearNamedBufferData(mSecondEdlSsbo, GL_RG32F, GL_RG, GL_FLOAT, &edlLevelClear);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Render pass
    glm::mat4 mvp = mCamera->projectionMatrix() * mCamera->viewMatrix();

    glUseProgram(mRenderProgram);
    glProgramUniformMatrix4fv(mRenderProgram, mRenderProgramMvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mRenderProgram, mRenderProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);

    uint32_t r = static_cast<uint32_t>(mParams.pointColor.r * 255.0f);
    uint32_t g = static_cast<uint32_t>(mParams.pointColor.g * 255.0f);
    uint32_t b = static_cast<uint32_t>(mParams.pointColor.b * 255.0f);
    uint32_t rgb = (r << 16) | (g << 8) | b;
    glProgramUniform1ui(mRenderProgram, mRenderProgramColorLocation, rgb);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFirstFramebufferSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Hole filling pass
    if (mParams.enableHoleFilling) {
        glUseProgram(mHoleFillingProgram);

        for (int iteration = 0; iteration < mParams.holeFillingIterations; iteration++) {
            glProgramUniform2i(mHoleFillingProgram, mHoleFillingProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
            glProgramUniform1i(mHoleFillingProgram, mHoleFillingProgramIterationLocation, iteration);
            glProgramUniform1f(mHoleFillingProgram, mHoleFillingProgramInfluenceLocation, mParams.holeFillingInfluence);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstFramebufferSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mSecondFramebufferSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mEmptyMaskSsbo);

            glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            std::swap(mFirstFramebufferSsbo, mSecondFramebufferSsbo);
        }
    }

    // Edl pass
    if (mParams.enableEdl) {
        glUseProgram(mEdlProgram);

        for (int level = 0; level < mParams.edlLevels; level++) {
            glProgramUniform2i(mEdlProgram, mEdlProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
            glProgramUniform1i(mEdlProgram, mEdlProgramLevelLocation, level);
            glProgramUniform1f(mEdlProgram, mEdlProgramShadingFactorLocation, mParams.shadingFactor);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstFramebufferSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFirstEdlSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mSecondEdlSsbo);

            glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            std::swap(mFirstEdlSsbo, mSecondEdlSsbo);
        }
    }

    // Resolve pass
    glUseProgram(mResolveProgram);
    glProgramUniform2i(mResolveProgram, mResolveProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
    glProgramUniform1i(mResolveProgram, mResolveProgramUseEdlLocation, mParams.enableEdl);
    glProgramUniform1f(mResolveProgram, mResolveProgramEdlShadingStrengthLocation, mParams.shadingStrength);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstFramebufferSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFirstEdlSsbo);
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

    glCreateBuffers(1, &mFirstFramebufferSsbo);
    glNamedBufferStorage(mFirstFramebufferSsbo, count * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mSecondFramebufferSsbo);
    glNamedBufferStorage(mSecondFramebufferSsbo, count * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mEmptyMaskSsbo);
    glNamedBufferStorage(mEmptyMaskSsbo, count * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mFirstEdlSsbo);
    glNamedBufferStorage(mFirstEdlSsbo, count * sizeof(glm::vec2), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mSecondEdlSsbo);
    glNamedBufferStorage(mSecondEdlSsbo, count * sizeof(glm::vec2), nullptr, GL_DYNAMIC_STORAGE_BIT);
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

    glDeleteBuffers(1, &mFirstFramebufferSsbo);
    glDeleteBuffers(1, &mSecondFramebufferSsbo);
    glDeleteBuffers(1, &mEmptyMaskSsbo);
    glDeleteBuffers(1, &mFirstEdlSsbo);
    glDeleteBuffers(1, &mSecondEdlSsbo);
    glDeleteTextures(1, &mOutputTexture);

    createFramebuffer();
    createOutputTexture();
}
