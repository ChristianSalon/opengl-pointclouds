#include "poisson_reconstruction_renderer.h"

void PoissonReconstructionRenderer::reconstruct() {
    mVertices.clear();
    mIndices.clear();
    mMesh.clear();

    if (mOriginalPoints.empty()) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): Point set is empty");
    }
    std::cout << "PoissonReconstructionRenderer::reconstruct(): Input points: " << mOriginalPoints.size() << std::endl;
    Point_set mPoints = mOriginalPoints;

    // Remove outliers
    if (mPoissonParams->removeOutliers) {
        auto removed = CGAL::remove_outliers<CGAL::Sequential_tag>(
            mPoints, mPoissonParams->outlierNeighbors,
            mPoints.parameters().threshold_percent(mPoissonParams->outlierThreshold));
        mPoints.remove(removed, mPoints.end());
        mPoints.collect_garbage();

        std::cout << "PoissonReconstructionRenderer::reconstruct(): Removed outliers" << std::endl;
    }

    // Calculate average spacing between points
    double spacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(mPoints, 6);
    if (!std::isfinite(spacing) || spacing <= 0.0) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): Invalid spacing");
    }
    std::cout << "PoissonReconstructionRenderer::reconstruct(): Spacing: " << spacing << std::endl;

    // Simplify point cloud
    if (mPoissonParams->simplify) {
        auto simplified = CGAL::grid_simplify_point_set(mPoints, mPoissonParams->simplifyRatio * spacing);
        mPoints.remove(simplified, mPoints.end());
        mPoints.collect_garbage();

        std::cout << "PoissonReconstructionRenderer::reconstruct(): Simplified point cloud" << std::endl;
    }

    // Smooth point cloud
    if (mPoissonParams->smooth) {
        CGAL::jet_smooth_point_set<CGAL::Sequential_tag>(mPoints, mPoissonParams->smoothNeighbors);
        std::cout << "PoissonReconstructionRenderer::reconstruct(): Smoothed point cloud" << std::endl;
    }

    // Estimate normals
    if (mPoissonParams->estimateNormals) {
        CGAL::jet_estimate_normals<CGAL::Sequential_tag>(mPoints, mPoissonParams->estimateNormalNeighbors);
        auto unorientedBegin = CGAL::mst_orient_normals(mPoints, mPoissonParams->estimateNormalNeighbors);
        mPoints.remove(unorientedBegin, mPoints.end());
        mPoints.collect_garbage();

        std::cout << "PoissonReconstructionRenderer::reconstruct(): Estimated normals" << std::endl;
    }

    if (mPoints.empty()) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): All points removed during orientation");
    }
    std::cout << "PoissonReconstructionRenderer::reconstruct(): Points after orientation: " << mPoints.size()
              << std::endl;

    // Poisson surface reconstruction
    bool success = CGAL::poisson_surface_reconstruction_delaunay(mPoints.begin(), mPoints.end(), mPoints.point_map(),
                                                                 mPoints.normal_map(), mMesh, spacing);
    if (!success) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): Poisson reconstruction failed");
    }

    // Check if a mesh was generated
    if (mMesh.number_of_vertices() == 0) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): Reconstructed mesh has no vertices");
    }
    if (mMesh.number_of_faces() == 0) {
        throw std::runtime_error("PoissonReconstructionRenderer::reconstruct(): Reconstructed mesh has no faces");
    }
    std::cout << "PoissonReconstructionRenderer::reconstruct(): Mesh vertices: " << mMesh.number_of_vertices()
              << std::endl;
    std::cout << "PoissonReconstructionRenderer::reconstruct(): Mesh faces: " << mMesh.number_of_faces() << std::endl;

    // Hole filling
    if (mPoissonParams->fillHoles) {
        std::vector<Mesh::Halfedge_index> borderCycles;
        CGAL::Polygon_mesh_processing::extract_boundary_cycles(mMesh, std::back_inserter(borderCycles));

        for (Mesh::Halfedge_index h : borderCycles) {
            CGAL::Polygon_mesh_processing::triangulate_and_refine_hole(mMesh, h);
        }

        std::cout << "PoissonReconstructionRenderer::reconstruct(): Holes filled: " << borderCycles.size() << std::endl;
    }

    // Get normal map from .ply file
    std::optional<Mesh::Property_map<Mesh::Vertex_index, K::Vector_3>> normalMap =
        mMesh.property_map<Mesh::Vertex_index, K::Vector_3>("v:normal");
    if (!normalMap) {
        std::cout << "PoissonReconstructionRenderer::setPointCloud(): Point cloud is missing normal map" << std::endl;

        // Compute normals if missing
        normalMap = mMesh.add_property_map<Mesh::Vertex_index, K::Vector_3>("v:normal", K::Vector_3(0, 0, 0)).first;
        CGAL::Polygon_mesh_processing::compute_vertex_normals(mMesh, *normalMap);
    }

    // Get color map from .ply file
    std::optional<Mesh::Property_map<Mesh::Vertex_index, CGAL::IO::Color>> colorMap =
        mMesh.property_map<Mesh::Vertex_index, CGAL::IO::Color>("v:color");
    if (!colorMap) {
        std::cout << "PoissonReconstructionRenderer::setPointCloud(): Point cloud is missing color map" << std::endl;

        // Set default color of vertices
        colorMap =
            mMesh.add_property_map<Mesh::Vertex_index, CGAL::IO::Color>("v:color", CGAL::IO::Color(0, 0, 1, 1)).first;
    }

    // Create VBO
    for (auto v : mMesh.vertices()) {
        auto p = mMesh.point(v);
        auto n = (*normalMap)[v];
        auto c = (*colorMap)[v];

        mVertices.push_back(Vertex{glm::vec3(p.x(), p.y(), p.z()), glm::vec3(n.x(), n.y(), n.z()),
                                   glm::u8vec4(c.red(), c.green(), c.blue(), c.alpha())});
    }

    for (auto f : mMesh.faces()) {
        for (auto v : mMesh.vertices_around_face(mMesh.halfedge(f))) {
            mIndices.push_back(static_cast<uint32_t>(v));
        }
    }

    glNamedBufferData(mVerticesVbo, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(mIndicesVbo, mIndices.size() * sizeof(uint32_t), mIndices.data(), GL_DYNAMIC_DRAW);
}

Renderer::BoundingBox PoissonReconstructionRenderer::setPointCloud(const std::string &path) {
    std::ifstream stream(path, std::ios_base::binary);
    if (!stream) {
        throw std::runtime_error("PoissonReconstructionRenderer::setPointCloud(): Could not read .ply file " + path);
    }

    stream >> mOriginalPoints;
    if (mOriginalPoints.empty()) {
        throw std::runtime_error("PoissonReconstructionRenderer::setPointCloud(): Loaded point cloud is empty");
    }

    mHasColor = mOriginalPoints.has_property_map<CGAL::IO::Color>("v:color");
    
    auto cgalBbox = CGAL::bbox_3(mOriginalPoints.points().begin(), mOriginalPoints.points().end());
    
    BoundingBox bbox;
    bbox.center = glm::vec3(
        (cgalBbox.xmin() + cgalBbox.xmax()) * 0.5f,
        (cgalBbox.ymin() + cgalBbox.ymax()) * 0.5f,
        (cgalBbox.zmin() + cgalBbox.zmax()) * 0.5f
    );

    glm::vec3 minP(cgalBbox.xmin(), cgalBbox.ymin(), cgalBbox.zmin());
    glm::vec3 maxP(cgalBbox.xmax(), cgalBbox.ymax(), cgalBbox.zmax());
    bbox.radius = glm::distance(minP, maxP) * 0.5f;

    return bbox;
}

void PoissonReconstructionRenderer::exportMesh(const std::string &path) const {
    std::ofstream file(path, std::ios_base::binary);
    CGAL::IO::set_binary_mode(file);
    CGAL::IO::write_PLY(file, mMesh);
    file.close();
}
