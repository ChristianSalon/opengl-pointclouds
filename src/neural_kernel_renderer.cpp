#include "neural_kernel_renderer.h"
#include <iostream>
#include "marching_cubes_lookup.h"
#include <omp.h>

void NeuralKernelRenderer::initialize() {
    GLuint vs = createShader(VS_PATH, GL_VERTEX_SHADER);
    GLuint fs = createShader(FS_PATH, GL_FRAGMENT_SHADER);

    mProgram = glCreateProgram();
    glAttachShader(mProgram, vs);
    glAttachShader(mProgram, fs);
    glLinkProgram(mProgram);

    glCreateVertexArrays(1, &mVao);
    glCreateBuffers(1, &mVerticesVbo);
    glCreateBuffers(1, &mIndicesVbo);

    GLuint cs = createShader(COMPUTE_PATH, GL_COMPUTE_SHADER);
    mComputeProgram = glCreateProgram();
    glAttachShader(mComputeProgram, cs);
    glLinkProgram(mComputeProgram);
    glDeleteShader(cs);
}

void NeuralKernelRenderer::setPointCloud(std::shared_ptr<OctreeBuilder::OctreeNode> octree) {
    if (!octree)
        return;
    mRawPoints.clear();

    // Flatten Octree to get raw points
    std::vector<OctreeBuilder::OctreeNode *> queue = {octree.get()};
    while (!queue.empty()) {
        auto node = queue.back();
        queue.pop_back();
        for (const auto &pos : node->positions)
            mRawPoints.push_back(glm::vec3(pos));
        for (auto &child : node->children)
            if (child)
                queue.push_back(child.get());
    }

    reconstructNeuralSurface();
}

void NeuralKernelRenderer::reconstructNeuralSurface() {
    if (mRawPoints.empty())
        return;
    int res = mGridRes;
    std::cout << "[AI-NKS] Reconstructing Neural Surface..." << std::endl;

    // Calculate Bounds and Prepare Data
    glm::vec3 minB(1e10), maxB(-1e10);
    std::vector<glm::vec4> gpuPoints;
    for (auto &p : mRawPoints) {
        minB = glm::min(minB, p);
        maxB = glm::max(maxB, p);
        gpuPoints.push_back(glm::vec4(p, 1.0f));
    }
    // Pad the bounds so the bunny doesn't touch the walls
    minB -= 0.1f;
    maxB += 0.1f;

    // Setup GPU Buffers
    if (mPointSsbo == 0)
        glGenBuffers(1, &mPointSsbo);
    glNamedBufferData(mPointSsbo, gpuPoints.size() * sizeof(glm::vec4), gpuPoints.data(), GL_STATIC_DRAW);

    int totalVoxels = mGridRes * mGridRes * mGridRes;
    if (mFieldSsbo == 0)
        glGenBuffers(1, &mFieldSsbo);
    glNamedBufferData(mFieldSsbo, totalVoxels * sizeof(float), nullptr, GL_STREAM_READ);

    // 3. Dispatch Compute Job to GPU
    glUseProgram(mComputeProgram);
    glUniform1i(glGetUniformLocation(mComputeProgram, "u_PointCount"), (int)gpuPoints.size());
    glUniform1f(glGetUniformLocation(mComputeProgram, "u_Sigma"), mSigma);
    glUniform3fv(glGetUniformLocation(mComputeProgram, "u_MinBounds"), 1, &minB[0]);
    glUniform3fv(glGetUniformLocation(mComputeProgram, "u_MaxBounds"), 1, &maxB[0]);
    glUniform1i(glGetUniformLocation(mComputeProgram, "u_Res"), mGridRes);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointSsbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mFieldSsbo);

    int groups = (mGridRes + 7) / 8;
    glDispatchCompute(groups, groups, groups);

    // Sync: Make sure GPU is done before we read
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    // Download Result
    std::vector<float> field(totalVoxels);
    glGetNamedBufferSubData(mFieldSsbo, 0, totalVoxels * sizeof(float), field.data());

    std::cout << "[AI-NKS] Neural field generated on GPU." << std::endl;

    // Marching Cubes using lookup tables
    mVertices.clear();
    mIndices.clear();

    std::cout << "[AI-NKS] Parallel extracting triangles..." << std::endl;

    std::vector<Vertex> globalVertices;
    std::vector<uint32_t> globalIndices;

    #pragma omp parallel
    {
        std::vector<Vertex> localVertices;
        std::vector<uint32_t> localIndices;

    #pragma omp for collapse(2)
        for (int z = 0; z < res - 1; ++z) {
            for (int y = 0; y < res - 1; ++y) {
                for (int x = 0; x < res - 1; ++x) {
                    float val[8];
                    glm::vec3 pos[8];

                    // Find the 8 corners of the voxel
                    for (int i = 0; i < 8; ++i) {
                        int ix = x + ((i ^ (i >> 1)) & 1);
                        int iy = y + ((i >> 1) & 1);
                        int iz = z + ((i >> 2) & 1);
                        val[i] = field[iz * res * res + iy * res + ix];
                        pos[i] = minB + (maxB - minB) * (glm::vec3(ix, iy, iz) / float(res - 1));
                    }

                    int cubeIndex = 0;
                    for (int i = 0; i < 8; ++i) {
                        if (val[i] > mThreshold)
                            cubeIndex |= (1 << i);
                    }

                    if (edgeTable[cubeIndex] == 0)
                        continue;

                    // Calculate the 12 Edges!
                    glm::vec3 vertList[12];
                    if (edgeTable[cubeIndex] & 1)
                        vertList[0] = (pos[0] + pos[1]) * 0.5f;
                    if (edgeTable[cubeIndex] & 2)
                        vertList[1] = (pos[1] + pos[2]) * 0.5f;
                    if (edgeTable[cubeIndex] & 4)
                        vertList[2] = (pos[2] + pos[3]) * 0.5f;
                    if (edgeTable[cubeIndex] & 8)
                        vertList[3] = (pos[3] + pos[0]) * 0.5f;
                    if (edgeTable[cubeIndex] & 16)
                        vertList[4] = (pos[4] + pos[5]) * 0.5f;
                    if (edgeTable[cubeIndex] & 32)
                        vertList[5] = (pos[5] + pos[6]) * 0.5f;
                    if (edgeTable[cubeIndex] & 64)
                        vertList[6] = (pos[6] + pos[7]) * 0.5f;
                    if (edgeTable[cubeIndex] & 128)
                        vertList[7] = (pos[7] + pos[4]) * 0.5f;
                    if (edgeTable[cubeIndex] & 256)
                        vertList[8] = (pos[0] + pos[4]) * 0.5f;
                    if (edgeTable[cubeIndex] & 512)
                        vertList[9] = (pos[1] + pos[5]) * 0.5f;
                    if (edgeTable[cubeIndex] & 1024)
                        vertList[10] = (pos[2] + pos[6]) * 0.5f;
                    if (edgeTable[cubeIndex] & 2048)
                        vertList[11] = (pos[3] + pos[7]) * 0.5f;

                    // Build the actual triangles using vertList
                    for (int i = 0; triangleTable[cubeIndex][i] != -1; i += 3) {
                        for (int j = 0; j < 3; ++j) {
                            Vertex v;
                            // Pull from the 12 edges, not the 8 corners!
                            v.position = vertList[triangleTable[cubeIndex][i + j]];
                            v.normal = glm::normalize(v.position - (minB + maxB) * 0.5f);
                            v.color = glm::u8vec4(180, 180, 180, 255);

                            localIndices.push_back((uint32_t)localVertices.size());
                            localVertices.push_back(v);
                        }
                    }
                }
            }
        }

        #pragma omp critical
        {
            uint32_t offset = (uint32_t)globalVertices.size();
            for (auto &idx : localIndices)
                idx += offset;
            globalVertices.insert(globalVertices.end(), localVertices.begin(), localVertices.end());
            globalIndices.insert(globalIndices.end(), localIndices.begin(), localIndices.end());
        }
    }

    mVertices = std::move(globalVertices);
    mIndices = std::move(globalIndices);

    setupBuffers();
    std::cout << "[AI-NKS] Done! Generated " << mIndices.size() / 3 << " triangles." << std::endl;
}


size_t NeuralKernelRenderer::draw() {
    if (mIndices.empty())
        return 0;

    // prevents "smearing"
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Ensure we can see the bunny's shape correctly
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(mProgram);

    // Set Camera Matrices
    glm::mat4 view = mCamera->viewMatrix();
    glm::mat4 proj = mCamera->projectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(mProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mProgram, "proj"), 1, GL_FALSE, &proj[0][0]);

    // Set missing color uniforms
    glUniform1i(glGetUniformLocation(mProgram, "useDefaultColor"), mParams->useDefaultColor ? 1 : 0);
    glUniform4f(glGetUniformLocation(mProgram, "defaultColor"), mParams->pointColor.r, mParams->pointColor.g,
                mParams->pointColor.b, 1.0f);

    // Perform Draw
    glBindVertexArray(mVao);
    glDrawElements(GL_TRIANGLES, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);

    // Cleanup
    glBindVertexArray(0);
    glUseProgram(0);

    return mIndices.size() / 3;
}


void NeuralKernelRenderer::destroy() {
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mVerticesVbo);
    glDeleteBuffers(1, &mIndicesVbo);
    glDeleteVertexArrays(1, &mVao);
}

void NeuralKernelRenderer::setupBuffers() {
    glNamedBufferData(mVerticesVbo, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(mIndicesVbo, mIndices.size() * sizeof(uint32_t), mIndices.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(mVao, 0, mVerticesVbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(mVao, mIndicesVbo);

    // Position
    glEnableVertexArrayAttrib(mVao, 0);
    glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(mVao, 0, 0);

    // Normal
    glEnableVertexArrayAttrib(mVao, 1);
    glVertexArrayAttribFormat(mVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(mVao, 1, 0);

    // Color
    glEnableVertexArrayAttrib(mVao, 2);
    glVertexArrayAttribFormat(mVao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Vertex, color));
    glVertexArrayAttribBinding(mVao, 2, 0);
}