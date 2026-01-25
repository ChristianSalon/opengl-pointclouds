/**
 * @file perspective_camera.cpp
 * @author Christian Saloň
 */

#include "perspective_camera.h"

/**
 * @brief PerspectiveCamera constructor
 *
 * @param position Camera position
 * @param rotation Camera rotation
 * @param fov Field of view in y axis direction
 * @param aspectRatio Aspect ratio (x:y)
 * @param nearPlane Near clipping plane
 * @param farPlane Far cipping plane
 */
PerspectiveCamera::PerspectiveCamera(glm::vec3 position,
                                     glm::vec3 rotation,
                                     float fov,
                                     float aspectRatio,
                                     float nearPlane,
                                     float farPlane)
    : BaseCamera{position, rotation} {
    this->setProjection(fov, aspectRatio, nearPlane, farPlane);
}

/**
 * @brief PerspectiveCamera constructor
 *
 * @param position Camera position
 * @param fov Field of view in y axis direction
 * @param aspectRatio Aspect ratio (x:y)
 * @param nearPlane Near clipping plane
 * @param farPlane Far cipping plane
 */
PerspectiveCamera::PerspectiveCamera(glm::vec3 position, float fov, float aspectRatio, float nearPlane, float farPlane)
    : BaseCamera{position, glm::vec3(0.f, 0.f, 0.f)} {
    this->setProjection(fov, aspectRatio, nearPlane, farPlane);
}

/**
 * @brief Update projection matrix based on camera properties
 *
 * @param fov Field of view in y axis direction
 * @param aspectRatio Aspect ratio (x:y)
 * @param nearPlane Near clipping plane
 * @param farPlane Far cipping plane
 */
void PerspectiveCamera::setProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    this->_fov = fov;
    this->_aspectRatio = aspectRatio;
    this->_nearPlane = nearPlane;
    this->_farPlane = farPlane;

    this->_projectionMatrix =
        glm::perspective(glm::radians(this->_fov), this->_aspectRatio, this->_nearPlane, this->_farPlane);

    _frustum = _getFrustum();
}

/**
 * @brief Update view matrix after changing camera properties
 */
void PerspectiveCamera::_setViewMatrix() {
    BaseCamera::_setViewMatrix();

    _frustum = _getFrustum();
}

/**
 * @brief Compute new frustum based on view and projection
 *
 * @return New frustum of camera
 */
PerspectiveCamera::Frustum PerspectiveCamera::_getFrustum() const {
    float halfFarVerticalSide = _farPlane * tanf(glm::radians(_fov) * 0.5f);
    float halfFarHorizontalSide = halfFarVerticalSide * _aspectRatio;
    glm::vec3 directionMultFar = _farPlane * _direction;

    Frustum frustum;
    frustum.planes[0] = Plane{_position, glm::cross(_up, directionMultFar + _right * halfFarHorizontalSide)};
    frustum.planes[1] = Plane{_position, glm::cross(directionMultFar - _right * halfFarHorizontalSide, _up)};
    frustum.planes[2] = Plane{_position, glm::cross(directionMultFar + _up * halfFarVerticalSide, _right)};
    frustum.planes[3] = Plane{_position, glm::cross(_right, directionMultFar - _up * halfFarVerticalSide)};
    frustum.planes[4] = Plane{_position + _nearPlane * _direction, _direction};
    frustum.planes[5] = Plane{_position + directionMultFar, -_direction};

    return frustum;
}
