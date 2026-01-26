#pragma once

#include <array>
#include <memory>
#include <random>
#include <vector>

#include <glm/glm.hpp>

class OctreeBuilder {
public:
    static constexpr size_t MAX_POINTS_IN_NODE = 10'000;

    struct Cube {
        glm::vec3 center;
        float halfSize;
    };

    struct OctreeNode {
        size_t id;
        Cube boundingCube;
        size_t pointCount;
        std::vector<glm::vec4> positions;
        std::vector<glm::u8vec4> colors;
        std::array<std::unique_ptr<OctreeNode>, 8> children = {nullptr};
        bool isLeaf = true;
    };

protected:
    size_t mNextId{0};

public:
    OctreeBuilder() {}
    ~OctreeBuilder() {}

    std::unique_ptr<OctreeNode> build(const std::vector<glm::vec4> &&positions,
                                      const std::vector<glm::u8vec4> &&colors);

protected:
    std::unique_ptr<OctreeNode> buildNode(size_t id,
                                          Cube boundingCube,
                                          std::vector<glm::vec4> &positions,
                                          std::vector<glm::u8vec4> &colors);
    Cube getBoundingCube(const std::vector<glm::vec4> &positions);
    glm::vec3 getChildCenter(glm::vec3 parentCenter, float childHalfSize, int octant);

    int getRandomInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 generator(rd());

        std::uniform_int_distribution<> distribution(min, max);
        return distribution(generator);
    }
};
