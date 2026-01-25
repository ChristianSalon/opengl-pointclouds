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

    processOctreeVertices();

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

size_t GlPointsRenderer::draw() {
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

    // Factor which converts world space to screen space
    float fovRad = glm::radians(mCamera->fov());
    float sseFactor = mWindowHeight / (2.0f * tanf(fovRad * 0.5f));

    size_t totalPointCount = 0;

    // Traverse octree
    std::queue<OctreeBuilder::OctreeNode *> open;
    open.push(mOctree.get());

    while (!open.empty()) {
        OctreeBuilder::OctreeNode *node = open.front();
        open.pop();

        // Frustum culling
        if (!mCamera->frustum().isCubeVisible(node->boundingCube.center, node->boundingCube.halfSize)) {
            continue;
        }

        // Draw current node
        if (node->pointCount > 0) {
            totalPointCount += node->pointCount;
            glDrawArrays(GL_POINTS, mOctreeInfo[node->id].offset, node->pointCount);
        }

        if (node->isLeaf) continue;

        // Calculate screen space error
        float distance = std::max(glm::distance(mCamera->position(), node->boundingCube.center), 0.01f);
        float screenSpaceError = (node->boundingCube.halfSize / distance) * sseFactor;

        // Draw children based on SSE
        if (screenSpaceError > mParams->maxPixelError) {
            for (auto &child : node->children) {
                if (child) {
                    open.push(child.get());
                }
            }
        }
    }

    return totalPointCount;
}

void GlPointsRenderer::processOctreeVertices() {
    mOctreeInfo.clear();
    mVertexPositions.clear();
    mVertexColors.clear();

    size_t totalPointCount = 0;

    std::queue<OctreeBuilder::OctreeNode *> open;
    open.push(mOctree.get());

    while (!open.empty()) {
        OctreeBuilder::OctreeNode *node = open.front();
        open.pop();

        mOctreeInfo.insert({node->id, OctreeNodeInfo{node->pointCount, totalPointCount}});
        mVertexPositions.insert(mVertexPositions.end(), std::make_move_iterator(node->positions.begin()), std::make_move_iterator(node->positions.end()));
        mVertexColors.insert(mVertexColors.end(), std::make_move_iterator(node->colors.begin()), std::make_move_iterator(node->colors.end()));

        node->positions.clear();
        node->colors.clear();

        totalPointCount += node->pointCount;

        if (node->isLeaf) continue;

        for (auto &child : node->children) {
            if (child) {
                open.push(child.get());
            }
        }
    }
}
