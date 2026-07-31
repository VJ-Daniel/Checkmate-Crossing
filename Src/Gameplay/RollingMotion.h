#pragma once

#include <glm.hpp>

#include "Transform3D.h"

/// Which axis a rolling object is allowed to spin about.
enum class RollAxisMode
{
    /// Spin about whichever horizontal axis is perpendicular to travel. Right
    /// for a ball, which has no preferred direction.
    FromTravel,

    /// Only ever spin about world X. For a log lying across the lanes: its
    /// length runs along X, so that is the only axis it can turn on. Without
    /// this a log sent along X would spin about Z like a drill bit.
    AboutX,

    /// Only ever spin about world Z, for a log lying the other way.
    AboutZ
};

/// Turns distance travelled into a rolling rotation.
///
/// Rolling without slipping means the surface covers exactly as much ground as
/// the object moves, which is
///
///     angle in radians = distance travelled / radius
///
/// Driving the spin from distance rather than a fixed rate is the whole point.
/// A rate picked by eye only matches at one speed; at any other the object
/// visibly skates or scrubs, and that reads as sliding no matter how round the
/// silhouette is.
///
/// This holds visual state only. It never moves anything and knows nothing
/// about collision - callers pass in the distance they already moved, and take
/// the rotation back out. Collision bounds stay wherever the eventual physics
/// code puts them.
///
/// Nothing in the project moves yet, so nothing drives this. It exists so that
/// when hazards do start moving, the rolling is already correct.
class RollingMotion
{
public:

    explicit RollingMotion(float radius = 0.5f);

    void SetRadius(float radius);

    float GetRadius() const;

    void SetAxisMode(RollAxisMode mode);

    RollAxisMode GetAxisMode() const;

    /// Accumulates the roll produced by one frame of travel.
    ///
    /// Takes the movement delta rather than a speed, so reversing direction
    /// reverses the spin on its own: the delta simply changes sign. Vertical
    /// movement is ignored, since only travel across the ground rolls
    /// anything.
    void Advance(const glm::vec3& travelDelta);

    /// Convenience for callers that have a velocity and a timestep.
    void Advance(const glm::vec3& velocity, float deltaTime);

    /// Accumulated rotation as Euler degrees, ready for Transform3D.
    glm::vec3 GetRotationDegrees() const;

    /// Writes the accumulated rotation onto a transform, leaving its position
    /// and scale alone.
    void ApplyTo(Transform3D& transform) const;

    void Reset();

    /// Total spin about each axis so far, in radians. Mostly for tests.
    float GetAngleAboutX() const;

    float GetAngleAboutZ() const;

private:

    float radius;

    RollAxisMode axisMode;

    /// Accumulated separately per axis.
    ///
    /// For travel along a single lane axis - which is all this game has -
    /// this is exact. Genuinely diagonal travel would need a quaternion,
    /// because two successive rotations about different axes do not commute
    /// and Euler angles cannot represent the result. Worth knowing before
    /// anyone sends a boulder across a lane at an angle.
    float angleAboutX;

    float angleAboutZ;
};
