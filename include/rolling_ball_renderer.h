#pragma once

#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <iterator>

// CGAL core
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Point_set_3/IO.h>
#include <CGAL/Surface_mesh.h>

// CGAL Algoritmy pre Rolling Ball 
#include <CGAL/Advancing_front_surface_reconstruction.h>

// CGAL Polygon Mesh Processing 
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

#include "triangle_mesh_renderer.h"

class RollingBallRenderer : public TriangleMeshRenderer {
private:
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef Kernel::Point_3 Point_3;
    typedef Kernel::Vector_3 Vector_3;
    typedef CGAL::Point_set_3<Point_3, Vector_3> Point_set;
    typedef CGAL::Surface_mesh<Point_3> Mesh;

protected:
    Point_set mPoints;

public:
    RollingBallRenderer(std::shared_ptr<PerspectiveCamera> camera, std::shared_ptr<Params> params)
        : TriangleMeshRenderer{camera, params} {}

    virtual ~RollingBallRenderer() {}

    void setPointCloud(const std::string &path);
    void reconstruct();

};