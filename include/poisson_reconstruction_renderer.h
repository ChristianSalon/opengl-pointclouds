#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <CGAL/Advancing_front_surface_reconstruction.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Point_set_3/IO.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Scale_space_reconstruction_3/Advancing_front_mesher.h>
#include <CGAL/Scale_space_reconstruction_3/Jet_smoother.h>
#include <CGAL/Scale_space_surface_reconstruction_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/grid_simplify_point_set.h>
#include <CGAL/jet_estimate_normals.h>
#include <CGAL/jet_smooth_point_set.h>
#include <CGAL/mst_orient_normals.h>
#include <CGAL/poisson_surface_reconstruction.h>
#include <CGAL/remove_outliers.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>

#include "path_utils.h"
#include "triangle_mesh_renderer.h"

class PoissonReconstructionRenderer : public TriangleMeshRenderer {
public:
    struct PoissonParams {
        bool removeOutliers = true;
        float outlierThreshold = 5.0f;
        int outlierNeighbors = 24;

        bool simplify = false;
        float simplifyRatio = 2.0f;
        int simplifyNeighbors = 24;

        bool smooth = true;
        int smoothNeighbors = 24;

        bool estimateNormals = true;
        int estimateNormalNeighbors = 24;

        bool fillHoles = false;
        int maxHoleEdges = 100;
        float maxHoleDiameter = 0.5f;
    };

private:
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef Kernel::FT FT;
    typedef Kernel::Point_3 Point_3;
    typedef Kernel::Vector_3 Vector_3;
    typedef Kernel::Sphere_3 Sphere_3;
    typedef CGAL::Point_set_3<Point_3, Vector_3> Point_set;
    typedef CGAL::Surface_mesh<Point_3> Mesh;
    typedef Mesh::Vertex_index vertex_descriptor;

protected:
    std::shared_ptr<PoissonParams> mPoissonParams;

    Point_set mOriginalPoints;
    Point_set mPoints;

public:
    PoissonReconstructionRenderer(std::shared_ptr<PerspectiveCamera> camera,
                                  std::shared_ptr<Params> params,
                                  std::shared_ptr<PoissonParams> poissonParams)
        : TriangleMeshRenderer{camera, params}, mPoissonParams{poissonParams} {}
    virtual ~PoissonReconstructionRenderer() {}

    virtual void setPointCloud(const std::string &path) override;

    virtual void reconstruct();
    virtual void exportMesh(const std::string &path) const;

    bool hasColor() const { return mHasColor; }
    virtual void setPointCloud(std::shared_ptr<OctreeBuilder::OctreeNode> octree) override {
        throw std::runtime_error("PoissonReconstructionRenderer::setPointCloud(octree) not supported");
    };
};
