#include "octree_builder.h"

std::unique_ptr<OctreeBuilder::OctreeNode> OctreeBuilder::build(const std::vector<glm::vec4> &&positions,
                                                                const std::vector<glm::u8vec4> &&colors) {
    if (positions.empty()) {
        return std::make_unique<OctreeNode>(0, Cube{glm::vec3(0.0f), 0.0f}, 0, std::vector<glm::vec4>{},
                                            std::vector<glm::u8vec4>{});
    }

    mNextId = 0;

    std::vector<glm::vec4> vertexPositions = std::move(positions);
    std::vector<glm::u8vec4> vertexColors = std::move(colors);

    Cube boundingCube = getBoundingCube(vertexPositions);
    std::unique_ptr<OctreeNode> root = buildNode(0, boundingCube, vertexPositions, vertexColors);

    return root;
}

std::unique_ptr<OctreeBuilder::OctreeNode> OctreeBuilder::buildNode(size_t id,
                                                                    Cube boundingCube,
                                                                    std::vector<glm::vec4> &positions,
                                                                    std::vector<glm::u8vec4> &colors) {
    int pointsToSample = std::min(MAX_POINTS_IN_NODE, positions.size());
    bool hasColor = colors.size() > 0;

    std::vector<glm::vec4> nodePositions;
    std::vector<glm::u8vec4> nodeColors;
    nodePositions.reserve(pointsToSample);
    if (hasColor)
        nodeColors.reserve(pointsToSample);

    // Random sampling
    for (size_t i = 0; i < pointsToSample; i++) {
        int randomIndex = getRandomInt(0, positions.size() - 1);

        nodePositions.push_back(positions[randomIndex]);
        if (hasColor)
            nodeColors.push_back(colors[randomIndex]);

        std::swap(positions[randomIndex], positions.back());
        positions.pop_back();

        if (hasColor) {
            std::swap(colors[randomIndex], colors.back());
            colors.pop_back();
        }
    }

    auto node = std::make_unique<OctreeNode>(id, boundingCube, pointsToSample, nodePositions, nodeColors);

    if (positions.size() == 0) {
        return node;
    }

    node->isLeaf = false;

    // Partion positions and colors
    glm::vec3 center = boundingCube.center;
    std::array<std::vector<glm::vec4>, 8> childPositions;
    std::array<std::vector<glm::u8vec4>, 8> childColors;
    for (int i = 0; i < 8; i++) {
        childPositions[i].reserve(positions.size() / 8);
        if (hasColor)
            childColors[i].reserve(positions.size() / 8);
    }

    for (size_t i = 0; i < positions.size(); i++) {
        int octantIndex = 0;
        if (positions[i].x >= center.x)
            octantIndex |= 1;
        if (positions[i].y >= center.y)
            octantIndex |= 2;
        if (positions[i].z >= center.z)
            octantIndex |= 4;

        childPositions[octantIndex].push_back(positions[i]);
        if (hasColor)
            childColors[octantIndex].push_back(colors[i]);
    }

    positions.clear();
    colors.clear();

    // Build children
    float childHalfSize = boundingCube.halfSize * 0.5f;
    for (int i = 0; i < 8; i++) {
        if (!childPositions[i].empty()) {
            Cube childCube{getChildCenter(center, childHalfSize, i), childHalfSize};
            node->children[i] = buildNode(++mNextId, childCube, childPositions[i], childColors[i]);
        }
    }

    return node;
}

OctreeBuilder::Cube OctreeBuilder::getBoundingCube(const std::vector<glm::vec4> &positions) {
    if (positions.empty()) {
        return Cube{glm::vec3(0.0f), 0.0f};
    }

    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());

    for (const glm::vec4 &p : positions) {
        min = glm::min(min, glm::vec3(p));
        max = glm::max(max, glm::vec3(p));
    }

    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 maxDimensions = max - min;
    float maxDimension = std::max({maxDimensions.x, maxDimensions.y, maxDimensions.z});

    return Cube{center, maxDimension * 0.5f};
}

glm::vec3 OctreeBuilder::getChildCenter(glm::vec3 parentCenter, float childHalfSize, int octant) {
    glm::vec3 center = parentCenter;

    center.x += (octant & 1) == 0 ? -childHalfSize : childHalfSize;
    center.y += (octant & 2) == 0 ? -childHalfSize : childHalfSize;
    center.z += (octant & 4) == 0 ? -childHalfSize : childHalfSize;

    return center;
}
