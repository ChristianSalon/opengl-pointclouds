/**
 * @file perspective_camera.h
 * @author Christian Saloň
 */

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "base_camera.h"

/**
 * @brief Camera using perspective projection
 */
class PerspectiveCamera : public BaseCamera {
private:
    float _fov;         /**< Field of view in the y axis direction in degrees */
    float _aspectRatio; /**< Aspect ratio of camera (x:y) */

    float _nearPlane; /**< Near clipping plane in pixels */
    float _farPlane;  /**< Far clipping plane in pixels */

public:
    PerspectiveCamera(glm::vec3 position,
                      glm::vec3 rotation,
                      float fov,
                      float aspectRatio,
                      float nearPlane,
                      float farPlane);
    PerspectiveCamera(glm::vec3 position, float fov, float aspectRatio, float nearPlane, float farPlane);

    void setProjection(float fov, float aspectRatio, float nearPlane, float farPlane);

    inline float fov() const { return _fov; }
    inline float aspectRatio() const { return _aspectRatio; }
    inline float near() const { return _nearPlane; }
    inline float far() const { return _farPlane; }

protected:
    virtual void _setViewMatrix() override;
    Frustum _getFrustum() const;
};
