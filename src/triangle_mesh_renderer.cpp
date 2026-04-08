#include "triangle_mesh_renderer.h"

void TriangleMeshRenderer::initialize() {
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

    glCreateVertexArrays(1, &mVao);

    glCreateBuffers(1, &mVerticesVbo);
    glCreateBuffers(1, &mIndicesVbo);

    glVertexArrayElementBuffer(mVao, mIndicesVbo);

    // Position attribute
    glEnableVertexArrayAttrib(mVao, 0);
    glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(mVao, 0, 0);

    // Normal attribute
    glEnableVertexArrayAttrib(mVao, 1);
    glVertexArrayAttribFormat(mVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(mVao, 1, 0);

    // Color attribute
    glEnableVertexArrayAttrib(mVao, 2);
    glVertexArrayAttribFormat(mVao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Vertex, color));
    glVertexArrayAttribBinding(mVao, 2, 0);

    glVertexArrayVertexBuffer(mVao, 0, mVerticesVbo, 0, sizeof(Vertex));

    glEnable(GL_DEPTH_TEST);
}

void TriangleMeshRenderer::destroy() {
    glDeleteProgram(mProgram);

    glDeleteShader(mVs);
    glDeleteShader(mFs);

    glDeleteBuffers(1, &mVerticesVbo);
    glDeleteBuffers(1, &mIndicesVbo);

    glDeleteVertexArrays(1, &mVao);
}

size_t TriangleMeshRenderer::draw() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(mProgram);

    glm::mat4 viewMatrix = mCamera->viewMatrix();
    glm::mat4 projectionMatrix = mCamera->projectionMatrix();
    glm::vec4 defaultColor(mParams->pointColor, 1.f);

    glProgramUniformMatrix4fv(mProgram, mViewLocation, 1, GL_FALSE, (float *)&viewMatrix);
    glProgramUniformMatrix4fv(mProgram, mProjLocation, 1, GL_FALSE, (float *)&projectionMatrix);
    glProgramUniform1i(mProgram, mUseDefaultColorLocation, mParams->useDefaultColor);
    glProgramUniform4fv(mProgram, mDefaultColorLocation, 1, glm::value_ptr(defaultColor));

    glBindVertexArray(mVao);

    glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0);

    return mIndices.size() / 3;
}

void TriangleMeshRenderer::setPointCloud(const std::string &path) {
    if (!CGAL::IO::read_PLY(path, mMesh)) {
        throw std::runtime_error("Could not read .ply file " + path);
    }

    // Get normal map from .ply file
    std::optional<Mesh::Property_map<Mesh::Vertex_index, K::Vector_3>> normalMap =
        mMesh.property_map<Mesh::Vertex_index, K::Vector_3>("v:normal");
    if (!normalMap) {
        std::cout << "TriangleMeshRenderer::setPointCloud(): Point cloud is missing normal map" << std::endl;

        // Compute normals if missing
        normalMap = mMesh.add_property_map<Mesh::Vertex_index, K::Vector_3>("v:normal", K::Vector_3(0, 0, 0)).first;
        CGAL::Polygon_mesh_processing::compute_vertex_normals(mMesh, *normalMap);
    }

    // Get color map from .ply file
    std::optional<Mesh::Property_map<Mesh::Vertex_index, CGAL::IO::Color>> colorMap =
        mMesh.property_map<Mesh::Vertex_index, CGAL::IO::Color>("v:color");
    if (!colorMap) {
        std::cout << "TriangleMeshRenderer::setPointCloud(): Point cloud is missing color map" << std::endl;

        // Set default color of vertices
        colorMap =
            mMesh.add_property_map<Mesh::Vertex_index, CGAL::IO::Color>("v:color", CGAL::IO::Color(0, 0, 1, 1)).first;
    }
    mHasColor = colorMap.has_value();

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