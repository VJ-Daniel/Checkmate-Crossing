/*
    ============================================================
    Checkmate Crossing - 3D Camera

    Builds the view and projection matrices for the tilted, fixed-angle
    battlefield view. The camera orbits a target point at a fixed pitch
    and yaw, so following the pawn later is a single SetTarget call.
    ============================================================
*/

#include "Camera3D.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Looking straight down would make the "up" vector ambiguous and the
    // view matrix would collapse, so the pitch stays inside these limits.
    constexpr float MinPitch = 1.0f;
    constexpr float MaxPitch = 89.0f;

    constexpr float MinOrthographicHeight = 0.1f;
}

Camera3D::Camera3D()
    : Camera3D(800.0f, 600.0f)
{
}

Camera3D::Camera3D(
    float screenWidth,
    float screenHeight)
    : screenWidth(screenWidth),
    screenHeight(screenHeight),
    target(0.0f),
    pitch(50.0f),
    yaw(0.0f),
    distance(15.0f),
    orthographicHeight(6.0f),
    fieldOfView(25.0f),
    nearPlane(0.1f),
    farPlane(200.0f),
    projectionMode(ProjectionMode::Orthographic),
    projection(1.0f),
    view(1.0f)
{
    Update();
}

void Camera3D::SetViewport(
    float screenWidth,
    float screenHeight)
{
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;

    UpdateProjection();
}

void Camera3D::SetTarget(const glm::vec3& target)
{
    this->target = target;

    UpdateView();
}

const glm::vec3& Camera3D::GetTarget() const
{
    return target;
}

void Camera3D::SetPitch(float degrees)
{
    pitch = std::clamp(degrees, MinPitch, MaxPitch);

    UpdateView();
}

float Camera3D::GetPitch() const
{
    return pitch;
}

void Camera3D::SetYaw(float degrees)
{
    yaw = degrees;

    UpdateView();
}

float Camera3D::GetYaw() const
{
    return yaw;
}

void Camera3D::SetDistance(float distance)
{
    this->distance = std::max(distance, 0.01f);

    UpdateView();
}

float Camera3D::GetDistance() const
{
    return distance;
}

void Camera3D::SetOrthographicHeight(float height)
{
    orthographicHeight = std::max(height, MinOrthographicHeight);

    UpdateProjection();
}

float Camera3D::GetOrthographicHeight() const
{
    return orthographicHeight;
}

void Camera3D::SetFieldOfView(float degrees)
{
    fieldOfView = std::clamp(degrees, 1.0f, 179.0f);

    UpdateProjection();
}

float Camera3D::GetFieldOfView() const
{
    return fieldOfView;
}

void Camera3D::SetClipPlanes(float nearPlane, float farPlane)
{
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;

    UpdateProjection();
}

void Camera3D::SetProjectionMode(ProjectionMode mode)
{
    projectionMode = mode;

    UpdateProjection();
}

ProjectionMode Camera3D::GetProjectionMode() const
{
    return projectionMode;
}

glm::vec3 Camera3D::GetPosition() const
{
    const float pitchRadians = glm::radians(pitch);
    const float yawRadians = glm::radians(yaw);

    // Offset from the target: up by sin(pitch), back by cos(pitch), then
    // swung around the target by the yaw.
    //
    // With the project's values (pitch 50.2, yaw 0, distance 15.62) this
    // produces exactly the offset the GDD suggests: (0, 12, 10).
    const glm::vec3 direction(
        std::sin(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::cos(yawRadians) * std::cos(pitchRadians));

    return target + direction * distance;
}

const glm::mat4& Camera3D::GetViewMatrix() const
{
    return view;
}

const glm::mat4& Camera3D::GetProjectionMatrix() const
{
    return projection;
}

CameraGroundBounds Camera3D::GetOrthographicGroundBounds(
    float groundHeight) const
{
    CameraGroundBounds bounds;

    const float aspect = (screenWidth > 0.0f && screenHeight > 0.0f)
        ? (screenWidth / screenHeight)
        : 1.0f;

    const float halfHeight = orthographicHeight * 0.5f;
    const float halfWidth = halfHeight * aspect;

    const glm::vec3 forward = glm::normalize(target - GetPosition());
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    const glm::vec3 cameraUp = glm::normalize(glm::cross(right, forward));

    bool firstCorner = true;

    for (int horizontalSide = -1;
        horizontalSide <= 1;
        horizontalSide += 2)
    {
        for (int verticalSide = -1;
            verticalSide <= 1;
            verticalSide += 2)
        {
            const glm::vec3 corner =
                target +
                right * (halfWidth * static_cast<float>(horizontalSide)) +
                cameraUp * (halfHeight * static_cast<float>(verticalSide));

            // Pitch is clamped away from zero, so forward.y is safely
            // non-zero. All orthographic corner rays share this direction.
            const float distanceToGround =
                (groundHeight - corner.y) / forward.y;

            const glm::vec3 groundPoint =
                corner + forward * distanceToGround;

            if (firstCorner)
            {
                bounds.minX = bounds.maxX = groundPoint.x;
                bounds.minZ = bounds.maxZ = groundPoint.z;
                firstCorner = false;
                continue;
            }

            bounds.minX = std::min(bounds.minX, groundPoint.x);
            bounds.maxX = std::max(bounds.maxX, groundPoint.x);
            bounds.minZ = std::min(bounds.minZ, groundPoint.z);
            bounds.maxZ = std::max(bounds.maxZ, groundPoint.z);
        }
    }

    return bounds;
}

void Camera3D::Update()
{
    UpdateProjection();
    UpdateView();
}

void Camera3D::UpdateProjection()
{
    const float aspect = (screenWidth > 0.0f && screenHeight > 0.0f)
        ? (screenWidth / screenHeight)
        : 1.0f;

    if (projectionMode == ProjectionMode::Orthographic)
    {
        // The vertical size is fixed and the horizontal size follows the
        // window shape, so the framing never stretches when resized.
        const float halfHeight = orthographicHeight * 0.5f;
        const float halfWidth = halfHeight * aspect;

        projection = glm::ortho(
            -halfWidth,
            halfWidth,
            -halfHeight,
            halfHeight,
            nearPlane,
            farPlane);
    }
    else
    {
        projection = glm::perspective(
            glm::radians(fieldOfView),
            aspect,
            nearPlane,
            farPlane);
    }
}

void Camera3D::UpdateView()
{
    view = glm::lookAt(
        GetPosition(),
        target,
        glm::vec3(0.0f, 1.0f, 0.0f));
}
