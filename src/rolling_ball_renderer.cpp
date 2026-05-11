#include "rolling_ball_renderer.h"
#include <CGAL/Alpha_shape_3.h>
#include <CGAL/Alpha_shape_cell_base_3.h>
#include <CGAL/Alpha_shape_vertex_base_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/advancing_front_surface_reconstruction.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Alpha_shape_vertex_base_3<Kernel> Vb;
typedef CGAL::Alpha_shape_cell_base_3<Kernel> Fb;
typedef CGAL::Triangulation_data_structure_3<Vb, Fb> Tds;
typedef CGAL::Delaunay_triangulation_3<Kernel, Tds> Triangulation_3;
typedef CGAL::Alpha_shape_3<Triangulation_3> Alpha_shape_3;

void RollingBallRenderer::reconstruct() {
    mVertices.clear();
    mIndices.clear();
    if (mPoints.empty())
        return;

    double radius = static_cast<double>(mParams->rollingBallRadius);
    std::vector<Point_3> soup_points;
    std::vector<std::array<std::size_t, 3>> soup_triangles;

    if (mParams->useStrictRollingBall) {
        //MODE: STRICT ROLLING BALL
        std::cout << "[RollingBallRenderer] Mode: Strict (Alpha Shape)" << std::endl;
        double alpha = radius * radius;
        Alpha_shape_3 as(mPoints.points().begin(), mPoints.points().end(), alpha, Alpha_shape_3::GENERAL);

        std::map<Triangulation_3::Vertex_handle, std::size_t> v_map;
        std::size_t current_idx = 0;

        for (auto it = as.finite_facets_begin(); it != as.finite_facets_end(); ++it) {
            if (as.classify(*it) == Alpha_shape_3::REGULAR) {
                std::array<std::size_t, 3> face_indices;
                for (int i = 0; i < 3; ++i) {
                    auto vh = it->first->vertex(as.vertex_triple_index(it->second, i));
                    if (v_map.find(vh) == v_map.end()) {
                        v_map[vh] = current_idx++;
                        soup_points.push_back(vh->point());
                    }
                    face_indices[i] = v_map[vh];
                }
                soup_triangles.push_back(face_indices);
            }
        }
    } else {
        //MODE: HEURISTIC ROLLING BALL (Advancing Front)
        std::cout << "[RollingBallRenderer] Mode: Heuristic (Advancing Front)" << std::endl;

        CGAL::advancing_front_surface_reconstruction(mPoints.points().begin(), mPoints.points().end(),
                                                     std::back_inserter(soup_triangles), radius);

        soup_points.reserve(mPoints.size());
        for (const auto &p : mPoints.points()) {
            soup_points.push_back(p);
        }
    }

    if (soup_triangles.empty()) {
        std::cout << "[RollingBallRenderer] No triangles generated with radius " << radius << std::endl;
        return;
    }


    CGAL::Polygon_mesh_processing::orient_polygon_soup(soup_points, soup_triangles);

    // Convert to Mesh
    Mesh mesh;
    CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(soup_points, soup_triangles, mesh);

    // Compute Normals
    auto normalMap = mesh.add_property_map<Mesh::Vertex_index, Vector_3>("v:normal", Vector_3(0, 0, 0)).first;
    CGAL::Polygon_mesh_processing::compute_vertex_normals(mesh, normalMap);

    // Fill OpenGL Buffers
    std::unordered_map<Mesh::Vertex_index, uint32_t> vMap;
    uint32_t gl_idx = 0;
    for (auto v : mesh.vertices()) {
        const Point_3 &p = mesh.point(v);
        const Vector_3 &n = normalMap[v];
        mVertices.push_back(
            {glm::vec3(p.x(), p.y(), p.z()), glm::vec3(-n.x(), -n.y(), -n.z()), glm::u8vec4(255, 255, 255, 255)});
        vMap[v] = gl_idx++;
    }

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices_around_face(mesh.halfedge(f))) {
            mIndices.push_back(vMap[v]);
        }
    }

    // GPU Upload
    glNamedBufferData(mVerticesVbo, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(mIndicesVbo, mIndices.size() * sizeof(uint32_t), mIndices.data(), GL_DYNAMIC_DRAW);

    std::cout << "[RollingBallRenderer] Finished. Triangles: " << soup_triangles.size() << std::endl;
}

void RollingBallRenderer::setPointCloud(const std::string &path) {
    std::ifstream stream(path, std::ios_base::binary);
    if (!stream) {
        throw std::runtime_error("RollingBallRenderer::setPointCloud(): Could not read .ply file " + path);
    }

    stream >> mPoints;
    if (mPoints.empty()) {
        throw std::runtime_error("RollingBallRenderer::setPointCloud(): Point set is empty after loading");
    }
}