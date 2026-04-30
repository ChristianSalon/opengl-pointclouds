#include "rolling_ball_renderer.h"

void RollingBallRenderer::reconstruct() {
    std::cout << "[RollingBallRenderer] Starting reconstruction..." << std::endl;

    mVertices.clear();
    mIndices.clear();
    if (mPoints.empty())
        return;

    std::vector<std::array<std::size_t, 3>> triangles;
    double radius = 0.05;

    // 1. Rekonštrukcia
    CGAL::advancing_front_surface_reconstruction(mPoints.points().begin(), mPoints.points().end(),
                                                 std::back_inserter(triangles), radius);

    // 2. Príprava bodov pre PMP
    std::vector<Point_3> soup_points;
    soup_points.reserve(mPoints.size());
    for (const auto &p : mPoints.points()) {
        soup_points.push_back(p);
    }

    // 3. Zjednotenie orientácie stien (aby tiene neboli čierne/prevrátené)
    CGAL::Polygon_mesh_processing::orient_polygon_soup(soup_points, triangles);

    // 4. Prevod na Surface_mesh
    Mesh mesh;
    CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(soup_points, triangles, mesh);

    // 5. Výpočet normál 
    auto normalMap = mesh.add_property_map<Mesh::Vertex_index, Vector_3>("v:normal", Vector_3(0, 0, 0)).first;
    CGAL::Polygon_mesh_processing::compute_vertex_normals(mesh, normalMap);

    // 6. Naplnenie dát pre OpenGL
    std::unordered_map<Mesh::Vertex_index, uint32_t> vMap;
    uint32_t idx = 0;

    for (auto v : mesh.vertices()) {
        const Point_3 &p = mesh.point(v);
        const Vector_3 &n = normalMap[v];  

        mVertices.push_back(
            Vertex{
                glm::vec3(p.x(), p.y(), p.z()), 
                glm::vec3(-n.x(),-n.y(),-n.z()), 
                glm::u8vec4(255, 255, 255, 255)});
        vMap[v] = idx++;
    }

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices_around_face(mesh.halfedge(f))) {
            mIndices.push_back(vMap[v]);
        }
    }

    // 7. GPU Upload
    glNamedBufferData(mVerticesVbo, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(mIndicesVbo, mIndices.size() * sizeof(uint32_t), mIndices.data(), GL_DYNAMIC_DRAW);

    std::cout << "[RollingBallRenderer] Finished. Triangles: " << triangles.size() << std::endl;
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