#include "base_camera.h"

BaseCamera::BaseCamera(glm::vec3 position, glm::vec3 rotation) : _position{position}, _rotation{rotation} {
    _updateProjection();
}

void BaseCamera::setPerspective(float fov, float aspect, float near, float far) {
    _fov = fov;
    _aspectRatio = aspect;
    _nearPlane = near;
    _farPlane = far;
    _updateProjection();
}

void BaseCamera::_setViewMatrix() {
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4{1.f}, glm::radians(this->_rotation.x), glm::vec3{1.f, 0.f, 0.f}) *
                               glm::rotate(glm::mat4{1.f}, glm::radians(this->_rotation.y), glm::vec3{0.f, 1.f, 0.f}) *
                               glm::rotate(glm::mat4{1.f}, glm::radians(this->_rotation.z), glm::vec3{0.f, 0.f, 1.f});

    this->_direction = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0)));
    this->_right =  glm::normalize(glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0)));
    this->_up =  glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0, 1, 0, 0)));
    this->_target = this->_position + this->_direction;

    this->_viewMatrix = glm::lookAt(this->_position, this->_target, this->_up);
}

void BaseCamera::_updateProjection() {
    _projectionMatrix = glm::perspective(glm::radians(_fov), _aspectRatio, _nearPlane, _farPlane);
}
