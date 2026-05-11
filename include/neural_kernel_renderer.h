#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>

#include "path_utils.h"
#include "renderer.h"

class NeuralKernelRenderer : public Renderer {
public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::u8vec4 color;
    };

    // Use existing triangle shaders to render the AI-generated mesh
    const std::filesystem::path VS_PATH = getExecutableDirectory() / "shaders" / "triangle_mesh.vert";
    const std::filesystem::path FS_PATH = getExecutableDirectory() / "shaders" / "triangle_mesh.frag";

private:
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    std::vector<glm::vec3> mRawPoints;

    GLuint mProgram{0};
    GLuint mVao{0}, mVerticesVbo{0}, mIndicesVbo{0};

    // AI Surface Parameters
    int mGridRes = 256;        // Increase for detail, decrease for speed
    float mSigma = 0.001f;     // The "Neural" influence radius
    float mThreshold = 0.95f;  // Surface detection level

public:
    NeuralKernelRenderer(std::shared_ptr<PerspectiveCamera> camera, std::shared_ptr<Params> params)
        : Renderer{camera, params} {}

    virtual void initialize() override;
    virtual void setPointCloud(std::shared_ptr<OctreeBuilder::OctreeNode> octree) override;
    virtual size_t draw() override;
    virtual void destroy() override;

    int &getGridRes() { return mGridRes; }
    float &getSigma() { return mSigma; }
    float &getThreshold() { return mThreshold; }

    void reconstructNeuralSurface();

private:
    void setupBuffers();
    GLuint mComputeProgram{0};
    GLuint mPointSsbo{0};  // Input to GPU
    GLuint mFieldSsbo{0};  // Output from GPU
    
    const std::filesystem::path COMPUTE_PATH = getExecutableDirectory() / "shaders" / "nks_field.comp";
};