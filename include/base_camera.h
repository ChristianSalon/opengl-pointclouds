/**
 * @file base_camera.h
 * @author Christian Saloň
 */

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

/**
 * @brief Base class for all cameras
 */
class BaseCamera {
public:
    struct Plane {
        glm::vec3 normal;
        float distance;

        Plane(const glm::vec3 &point, const glm::vec3 &norm) {
            normal = glm::normalize(norm);
            distance = glm::dot(normal, point);
        }
        Plane() = default;

        bool isPointAbove(const glm::vec3 &point) {
            return glm::dot(point, normal) - distance >= -0.01f;
        }
    };

    struct Frustum {
        Plane planes[6];

        bool isPointInside(const glm::vec3 &point) {
            return planes[0].isPointAbove(point) && planes[1].isPointAbove(point) && planes[2].isPointAbove(point) &&
                   planes[3].isPointAbove(point) && planes[4].isPointAbove(point) && planes[5].isPointAbove(point);
        }

        bool isCubeVisible(const glm::vec3 &center, float halfSize) {
            for (int i = 0; i < 6; i++) {
                // Find furthest point from plane
                glm::vec3 positive = center;
                positive.x += planes[i].normal.x >= 0.0f ? halfSize : -halfSize;
                positive.y += planes[i].normal.y >= 0.0f ? halfSize : -halfSize;
                positive.z += planes[i].normal.z >= 0.0f ? halfSize : -halfSize;

                if (!planes[i].isPointAbove(positive)) {
                    return false;
                }
            }

            return true;
        }
    };

protected:
    glm::vec3 _position{0.f, 0.f, 1.f};
    glm::vec3 _rotation{0.f, 0.f, 0.f};
    glm::vec3 _direction{0.f, 0.f, 1.f};
    glm::vec3 _target{0.f, 0.f, 0.f};
    glm::vec3 _up{0.f, 1.f, 0.f};
    glm::vec3 _right{1.f, 0.f, 0.f};

    glm::mat4 _viewMatrix{1.f};
    glm::mat4 _projectionMatrix{1.f};

    Frustum _frustum;

public:
    BaseCamera(glm::vec3 position, glm::vec3 rotation);

    void translate(glm::vec3 translation);
    void rotate(glm::vec3 rotation);

    void setPosition(glm::vec3 position);
    void setRotation(glm::vec3 rotation);
    void zoom(float delta);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;

    inline Frustum frustum() const { return _frustum; }
    inline glm::vec3 position() const { return this->_position; }
    inline glm::vec3 rotation() const { return this->_rotation; }
    inline glm::vec3 direction() const { return this->_direction; }
    inline glm::vec3 target() const { return this->_target; }
    inline glm::vec3 up() const { return this->_up; }
    inline glm::vec3 right() const { return this->_right; }

protected:
    virtual void _setViewMatrix();
};
