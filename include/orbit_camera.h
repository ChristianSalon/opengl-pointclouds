#pragma once

#include "base_camera.h"

class OrbitCamera : public BaseCamera {
private:
    float _distance{5.f};
    float _mouseSensitivity{0.15f};

public:
    OrbitCamera(glm::vec3 target, float distance) : BaseCamera(target, {0, 0, 0}) {
        _target = target;
        _distance = distance;
        _rotation = {0.f, 0.f, 0.f};
        _updateCameraVectors();
    }

    void zoom(float delta) override {
        _distance -= delta * (_distance * 0.1f);
        _distance = std::max(_distance, 0.1f);
        _updateCameraVectors();
    }

    void handleMouseInput(float dx, float dy, bool lmbDown, bool shiftDown) {
        if (!lmbDown)
            return;

        if (shiftDown) {
            _pan(dx, dy);
        } else {
            _orbit(dx, dy);
        }
    }

    void setTarget(glm::vec3 newTarget) {
        _target = newTarget;
        _updateCameraVectors();
    }

    void setDistance(float newDistance) {
        _distance = newDistance;
        _updateCameraVectors();
    }

private:
    void _orbit(float dx, float dy) {
        _rotation.y -= dx * _mouseSensitivity;
        _rotation.x -= dy * _mouseSensitivity;
        _rotation.x = glm::clamp(_rotation.x, -89.0f, 89.0f);
        _updateCameraVectors();
    }

    void _pan(float dx, float dy) {
        float panSpeed = _distance * 0.001f;
        glm::vec3 translation = (_right * -dx * panSpeed) + (_up * dy * panSpeed);

        _target += translation;
        _position += translation;
        _updateCameraVectors();
    }

    void _updateCameraVectors() {
        glm::mat4 rot = glm::rotate(glm::mat4{1.f}, glm::radians(_rotation.y), {0, 1, 0}) *
                        glm::rotate(glm::mat4{1.f}, glm::radians(_rotation.x), {1, 0, 0});

        _direction = glm::normalize(glm::vec3(rot * glm::vec4(0, 0, 1, 0)));
        _right = glm::normalize(glm::cross({0, 1, 0}, _direction));
        _up = glm::normalize(glm::cross(_direction, _right));

        _position = _target + (_direction * _distance);
        _viewMatrix = glm::lookAt(_position, _target, _up);
    }
};
