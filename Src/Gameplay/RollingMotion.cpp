/*
    ============================================================
    Checkmate Crossing - Rolling Motion

    Converts distance travelled into a rolling spin, so a ball or a log
    turns at exactly the rate its own surface would demand.
    ============================================================
*/

#include "RollingMotion.h"

#include <algorithm>

RollingMotion::RollingMotion(float radius)
    : radius(std::max(radius, 0.0001f)),
    axisMode(RollAxisMode::FromTravel),
    angleAboutX(0.0f),
    angleAboutZ(0.0f)
{
}

void RollingMotion::SetRadius(float radius)
{
    // Guarded rather than allowed to reach zero: the roll is a division by
    // this, and a zero radius would spin the object infinitely fast.
    this->radius = std::max(radius, 0.0001f);
}

float RollingMotion::GetRadius() const
{
    return radius;
}

void RollingMotion::SetAxisMode(RollAxisMode mode)
{
    axisMode = mode;
}

RollAxisMode RollingMotion::GetAxisMode() const
{
    return axisMode;
}

void RollingMotion::Advance(const glm::vec3& travelDelta)
{
    // For a ball on flat ground the spin axis is the ground normal crossed
    // with the direction of travel:
    //
    //     omega = (up x velocity) / radius
    //
    // Working that through for the two lane axes:
    //
    //     moving +X  ->  spins about -Z
    //     moving +Z  ->  spins about +X
    //
    // which is what the two signs below are.
    const float rollFromX = travelDelta.x / radius;
    const float rollFromZ = travelDelta.z / radius;

    switch (axisMode)
    {
    case RollAxisMode::FromTravel:
        angleAboutZ -= rollFromX;
        angleAboutX += rollFromZ;
        break;

    case RollAxisMode::AboutX:
        // A log across the lanes only turns on its own length, so travel
        // along that length is ignored rather than spun.
        angleAboutX += rollFromZ;
        break;

    case RollAxisMode::AboutZ:
        angleAboutZ -= rollFromX;
        break;
    }
}

void RollingMotion::Advance(const glm::vec3& velocity, float deltaTime)
{
    Advance(velocity * deltaTime);
}

glm::vec3 RollingMotion::GetRotationDegrees() const
{
    return glm::degrees(
        glm::vec3(angleAboutX, 0.0f, angleAboutZ));
}

void RollingMotion::ApplyTo(Transform3D& transform) const
{
    transform.SetRotation(GetRotationDegrees());
}

void RollingMotion::Reset()
{
    angleAboutX = 0.0f;
    angleAboutZ = 0.0f;
}

float RollingMotion::GetAngleAboutX() const
{
    return angleAboutX;
}

float RollingMotion::GetAngleAboutZ() const
{
    return angleAboutZ;
}
