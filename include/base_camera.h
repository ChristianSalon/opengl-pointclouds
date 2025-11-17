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
protected:
    glm::vec3 _position{0.f, 0.f, 1.f};
    glm::vec3 _rotation{0.f, 0.f, 0.f};
    glm::vec3 _direction{0.f, 0.f, 1.f};
    glm::vec3 _target{0.f, 0.f, 0.f};
    glm::vec3 _up{0.f, 1.f, 0.f};
    glm::vec3 _right{1.f, 0.f, 0.f};

    glm::mat4 _viewMatrix{1.f};       /**< Computed view matrix */
    glm::mat4 _projectionMatrix{1.f}; /**< Computed projection matrix */

public:
    BaseCamera(glm::vec3 position, glm::vec3 rotation);

    void translate(glm::vec3 translation);
    void rotate(glm::vec3 rotation);

    void setPosition(glm::vec3 position);
    void setRotation(glm::vec3 rotation);
    void zoom(float delta);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;

    inline glm::vec3 position() const { return this->_position; }
    inline glm::vec3 rotation() const { return this->_rotation; }
    inline glm::vec3 direction() const { return this->_direction; }
    inline glm::vec3 target() const { return this->_target; }
    inline glm::vec3 up() const { return this->_up; }
    inline glm::vec3 right() const { return this->_right; }

protected:
    void _setViewMatrix();
};
