#include <glm/gtc/type_ptr.hpp>

#include "high_quality_renderer.h"

void HighQualityRenderer::initialize() {
    mDepthCs = createShader(DEPTH_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mColorCs = createShader(COLOR_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mResolveCs = createShader(RESOLVE_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mHoleFillingCs = createShader(HOLE_FILLING_CS_PATH, GL_COMPUTE_SHADER);
    mEdlCs = createShader(EDL_CS_PATH, GL_COMPUTE_SHADER);
    mSecondResolveCs = createShader(SECOND_RESOLVE_PASS_CS_PATH, GL_COMPUTE_SHADER);
    mQuadVs = createShader(QUAD_VS_PATH, GL_VERTEX_SHADER);
    mQuadFs = createShader(QUAD_FS_PATH, GL_FRAGMENT_SHADER);

    mDepthProgram = glCreateProgram();
    mColorProgram = glCreateProgram();
    mResolveProgram = glCreateProgram();
    mHoleFillingProgram = glCreateProgram();
    mEdlProgram = glCreateProgram();
    mSecondResolveProgram = glCreateProgram();
    mQuadProgram = glCreateProgram();

    glAttachShader(mDepthProgram, mDepthCs);
    glAttachShader(mColorProgram, mColorCs);
    glAttachShader(mResolveProgram, mResolveCs);
    glAttachShader(mHoleFillingProgram, mHoleFillingCs);
    glAttachShader(mEdlProgram, mEdlCs);
    glAttachShader(mSecondResolveProgram, mSecondResolveCs);
    glAttachShader(mQuadProgram, mQuadVs);
    glAttachShader(mQuadProgram, mQuadFs);

    glLinkProgram(mDepthProgram);
    glLinkProgram(mColorProgram);
    glLinkProgram(mResolveProgram);
    glLinkProgram(mHoleFillingProgram);
    glLinkProgram(mEdlProgram);
    glLinkProgram(mSecondResolveProgram);
    glLinkProgram(mQuadProgram);

    mDepthProgramMvpLocation = glGetUniformLocation(mDepthProgram, "mvp");
    mDepthProgramFramebufferSizeLocation = glGetUniformLocation(mDepthProgram, "framebufferSize");

    mColorProgramColorLocation = glGetUniformLocation(mColorProgram, "color");
    mColorProgramMvpLocation = glGetUniformLocation(mColorProgram, "mvp");
    mColorProgramFramebufferSizeLocation = glGetUniformLocation(mColorProgram, "framebufferSize");

    mResolveProgramFramebufferSizeLocation = glGetUniformLocation(mResolveProgram, "framebufferSize");

    mHoleFillingProgramFramebufferSizeLocation = glGetUniformLocation(mHoleFillingProgram, "framebufferSize");
    mHoleFillingProgramIterationLocation = glGetUniformLocation(mHoleFillingProgram, "iteration");
    mHoleFillingProgramInfluenceLocation = glGetUniformLocation(mHoleFillingProgram, "influence");

    mEdlProgramFramebufferSizeLocation = glGetUniformLocation(mEdlProgram, "framebufferSize");
    mEdlProgramLevelLocation = glGetUniformLocation(mEdlProgram, "level");
    mEdlProgramShadingFactorLocation = glGetUniformLocation(mEdlProgram, "shadingFactor");

    mSecondResolveProgramFramebufferSizeLocation = glGetUniformLocation(mSecondResolveProgram, "framebufferSize");
    mSecondResolveProgramEdlShadingStrengthLocation= glGetUniformLocation(mSecondResolveProgram, "edlShadingStrength");
    mSecondResolveProgramUseEdlLocation = glGetUniformLocation(mSecondResolveProgram, "useEdl");

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
    glDeleteBuffers(1, &mFirstDepthSsbo);
    glDeleteBuffers(1, &mSecondDepthSsbo);
    glDeleteBuffers(1, &mFallbackSsbo);
    glDeleteBuffers(1, &mEmptyMaskSsbo);
    glDeleteBuffers(1, &mFirstEdlSsbo);
    glDeleteBuffers(1, &mSecondEdlSsbo);
    glDeleteTextures(1, &mFirstOutputTexture);
    glDeleteTextures(1, &mSecondOutputTexture);

    glDeleteProgram(mDepthProgram);
    glDeleteProgram(mColorProgram);
    glDeleteProgram(mResolveProgram);
    glDeleteProgram(mHoleFillingProgram);
    glDeleteProgram(mEdlProgram);
    glDeleteProgram(mSecondResolveProgram);
    glDeleteProgram(mQuadProgram);
}

void HighQualityRenderer::draw() {
    // Clear screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear framebuffer
    uint64_t framebufferClear = 0;
    glClearNamedBufferData(mFramebufferSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &framebufferClear);

    // Clear depth buffer
    uint32_t depthBufferClear = 0xffffffff;
    glClearNamedBufferData(mFirstDepthSsbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &depthBufferClear);
    glClearNamedBufferData(mSecondDepthSsbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &depthBufferClear);

    // Clear fallback buffer
    uint64_t fallbackClear = 0;
    glClearNamedBufferData(mFallbackSsbo, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, &fallbackClear);

    uint32_t errorMaskClear = 0;
    glClearNamedBufferData(mEmptyMaskSsbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &errorMaskClear);

    // Edl level info
    // depth, shading_term
    glm::vec2 edlLevelClear(0.0f, 0.0f);
    glClearNamedBufferData(mFirstEdlSsbo, GL_RG32F, GL_RG, GL_FLOAT, &edlLevelClear);
    glClearNamedBufferData(mSecondEdlSsbo, GL_RG32F, GL_RG, GL_FLOAT, &edlLevelClear);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Depth pass
    glm::mat4 mvp = mCamera->projectionMatrix() * mCamera->viewMatrix();

    glUseProgram(mDepthProgram);
    glProgramUniformMatrix4fv(mDepthProgram, mDepthProgramMvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mDepthProgram, mDepthProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFirstDepthSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Color pass
    glUseProgram(mColorProgram);
    glProgramUniformMatrix4fv(mColorProgram, mColorProgramMvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    glProgramUniform2i(mColorProgram, mColorProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);

    uint64_t r = static_cast<uint64_t>(mParams.pointColor.r * 255.f);
    uint64_t g = static_cast<uint64_t>(mParams.pointColor.g * 255.f);
    uint64_t b = static_cast<uint64_t>(mParams.pointColor.b * 255.f);
    uint64_t rgb = (r << 32) | (g << 16) | b;
    glProgramUniform1ui64ARB(mColorProgram, mColorProgramColorLocation, rgb);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointsSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFramebufferSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mFirstDepthSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mFallbackSsbo);

    glDispatchCompute((mVertexPositions.size() / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Resolve pass
    glUseProgram(mResolveProgram);
    glProgramUniform2i(mResolveProgram, mResolveProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFramebufferSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFallbackSsbo);
    glBindImageTexture(0, mFirstOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Hole filling pass
    if (mParams.enableHoleFilling) {
        glUseProgram(mHoleFillingProgram);

        for (int iteration = 0; iteration < mParams.holeFillingIterations; iteration++) {
            glProgramUniform2i(mHoleFillingProgram, mHoleFillingProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
            glProgramUniform1i(mHoleFillingProgram, mHoleFillingProgramIterationLocation, iteration);
            glProgramUniform1f(mHoleFillingProgram, mHoleFillingProgramInfluenceLocation, mParams.holeFillingInfluence);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstDepthSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mSecondDepthSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mEmptyMaskSsbo);

            glBindImageTexture(0, mFirstOutputTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(1, mSecondOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

            glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            std::swap(mFirstDepthSsbo, mSecondDepthSsbo);
            std::swap(mFirstOutputTexture, mSecondOutputTexture);
        }
    }

    // Edl pass
    if (mParams.enableEdl) {
        glUseProgram(mEdlProgram);

        for (int level = 0; level < mParams.edlLevels; level++) {
            glProgramUniform2i(mEdlProgram, mEdlProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
            glProgramUniform1i(mEdlProgram, mEdlProgramLevelLocation, level);
            glProgramUniform1f(mEdlProgram, mEdlProgramShadingFactorLocation, mParams.shadingFactor);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstDepthSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFirstEdlSsbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mSecondEdlSsbo);

            glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            std::swap(mFirstEdlSsbo, mSecondEdlSsbo);
        }

        // Second resolve pass - apply edl to final texture
        glUseProgram(mSecondResolveProgram);

        glProgramUniform2i(mSecondResolveProgram, mSecondResolveProgramFramebufferSizeLocation, mWindowWidth, mWindowHeight);
        glProgramUniform1i(mSecondResolveProgram, mSecondResolveProgramUseEdlLocation, mParams.enableEdl);
        glProgramUniform1f(mSecondResolveProgram, mSecondResolveProgramEdlShadingStrengthLocation, mParams.shadingStrength);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mFirstEdlSsbo);
        glBindImageTexture(0, mFirstOutputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        glDispatchCompute((mWindowWidth / 8) + 1, (mWindowHeight / 8) + 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Render fullscreen quad
    glDisable(GL_DEPTH_TEST);
    glUseProgram(mQuadProgram);

    glBindVertexArray(mQuadVao);
    glBindTextureUnit(0, mFirstOutputTexture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void HighQualityRenderer::createFramebuffer() {
    size_t count = mWindowWidth * mWindowHeight;

    glCreateBuffers(1, &mFramebufferSsbo);
    glNamedBufferStorage(mFramebufferSsbo, count * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mFirstDepthSsbo);
    glNamedBufferStorage(mFirstDepthSsbo, count * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mSecondDepthSsbo);
    glNamedBufferStorage(mSecondDepthSsbo, count * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mFallbackSsbo);
    glNamedBufferStorage(mFallbackSsbo, count * 2 * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mEmptyMaskSsbo);
    glNamedBufferStorage(mEmptyMaskSsbo, count * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mFirstEdlSsbo);
    glNamedBufferStorage(mFirstEdlSsbo, count * sizeof(glm::vec2), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &mSecondEdlSsbo);
    glNamedBufferStorage(mSecondEdlSsbo, count * sizeof(glm::vec2), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void HighQualityRenderer::createOutputTexture() {
    glCreateTextures(GL_TEXTURE_2D, 1, &mFirstOutputTexture);
    glTextureStorage2D(mFirstOutputTexture, 1, GL_RGBA8, mWindowWidth, mWindowHeight);

    glTextureParameteri(mFirstOutputTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(mFirstOutputTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glCreateTextures(GL_TEXTURE_2D, 1, &mSecondOutputTexture);
    glTextureStorage2D(mSecondOutputTexture, 1, GL_RGBA8, mWindowWidth, mWindowHeight);

    glTextureParameteri(mSecondOutputTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(mSecondOutputTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void HighQualityRenderer::setWindowDimensions(int width, int height) {
    Renderer::setWindowDimensions(width, height);

    glFinish();

    glDeleteBuffers(1, &mFramebufferSsbo);
    glDeleteBuffers(1, &mFirstDepthSsbo);
    glDeleteBuffers(1, &mSecondDepthSsbo);
    glDeleteBuffers(1, &mFallbackSsbo);
    glDeleteBuffers(1, &mEmptyMaskSsbo);
    glDeleteBuffers(1, &mFirstEdlSsbo);
    glDeleteBuffers(1, &mSecondEdlSsbo);
    glDeleteTextures(1, &mFirstOutputTexture);
    glDeleteTextures(1, &mSecondOutputTexture);

    createFramebuffer();
    createOutputTexture();
}
