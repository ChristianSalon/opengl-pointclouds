#pragma once

#include <iostream>
#include <vector>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Point_set_3/IO.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/helpers.h>

#include "path_utils.h"
#include "renderer.h"

class TriangleMeshRenderer : public Renderer {
private:
    typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
    typedef CGAL::Surface_mesh<K::Point_3> Mesh;

public:
    const std::filesystem::path VS_PATH = getExecutableDirectory() / "shaders" / "triangle_mesh.vert";
    const std::filesystem::path FS_PATH = getExecutableDirectory() / "shaders" / "triangle_mesh.frag";

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::u8vec4 color;
    };

protected:
    Mesh mMesh;

    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;

    GLuint mProgram;
    GLuint mVs, mFs;
    GLuint mVao, mVerticesVbo, mIndicesVbo;

    GLint mViewLocation;
    GLint mProjLocation;
    GLint mUseDefaultColorLocation;
    GLint mDefaultColorLocation;

    bool mHasColor{false};

public:
    TriangleMeshRenderer(std::shared_ptr<PerspectiveCamera> camera, std::shared_ptr<Params> params)
        : Renderer{camera, params} {}
    virtual ~TriangleMeshRenderer() {}

    virtual void initialize() override;
    virtual void destroy() override;
    virtual size_t draw() override;

    virtual void setPointCloud(const std::string &path);

    bool hasColor() const { return mHasColor; }
    virtual void setPointCloud(std::shared_ptr<OctreeBuilder::OctreeNode> octree) override {
        throw std::runtime_error("TriangleMeshRenderer::setPointCloud(octree) not supported");
    };
};
