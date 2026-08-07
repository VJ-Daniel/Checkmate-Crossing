/*
    ============================================================
    Checkmate Crossing - Moving Hazard
    Gameplay Programmer: Ayub
    Date: 2026

    See MovingHazard.h for an overview of each movement pattern.
    ============================================================
*/

#include "MovingHazard.h"

#include <algorithm>

#include <gtc/matrix_transform.hpp>

MovingHazard::MovingHazard(std::shared_ptr<GroundEntity> visual)
    : visual(std::move(visual))
{
}

void MovingHazard::SetLinearSweep(
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float speed,
    bool loop)
{
    pattern = HazardMovementPattern::LinearSweep;

    sweepStart = startGroundPosition;
    sweepEnd = endGroundPosition;
    sweepSpeed = speed;
    sweepLoop = loop;
    sweepForward = true;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);

    if (visual)
        visual->SetGroundPosition(sweepStart);
}

void MovingHazard::SetCurvedSweep(
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float speed,
    float curveOffset)
{
    pattern = HazardMovementPattern::CurvedSweep;

    curveStart = startGroundPosition;
    curveEnd = endGroundPosition;
    curveOffsetAmount = curveOffset;
    curveElapsed = 0.0f;

    // Duration derived from the straight-line distance and speed, so
    // "speed" still means roughly what it says despite the sideways bow.
    const float straightDistance = glm::length(endGroundPosition - startGroundPosition);
    curveDuration = (speed > 0.0f) ? (straightDistance / speed) : 1.0f;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);

    if (visual)
        visual->SetGroundPosition(curveStart);
}

void MovingHazard::SetArcProjectile(
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float duration,
    float arcHeightValue)
{
    pattern = HazardMovementPattern::ArcProjectile;

    arcStart = startGroundPosition;
    arcEnd = endGroundPosition;
    arcDuration = duration > 0.0f ? duration : 0.01f;
    arcHeight = arcHeightValue;
    arcElapsed = 0.0f;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);

    if (visual)
        visual->SetGroundPosition(arcStart);
}

void MovingHazard::SetWarningThenStrike(
    const glm::vec3& groundPosition,
    float warningDurationValue,
    float strikeDurationValue)
{
    pattern = HazardMovementPattern::WarningThenStrike;

    warningDuration = warningDurationValue;
    strikeDuration = strikeDurationValue;
    phaseElapsed = 0.0f;

    // Starts in the telegraph phase, not already dangerous.
    active = false;
    expired = false;
    velocity = glm::vec3(0.0f);

    if (visual)
        visual->SetGroundPosition(groundPosition);
}

void MovingHazard::SetTemporaryZone(
    const glm::vec3& groundPosition,
    float duration)
{
    pattern = HazardMovementPattern::TemporaryZone;

    zoneDuration = duration > 0.0f ? duration : 0.01f;
    zoneElapsed = 0.0f;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);

    if (visual)
        visual->SetGroundPosition(groundPosition);
}

void MovingHazard::SetFollowTarget(
    float maxSpeed,
    float detectionRange,
    float followDistanceValue)
{
    pattern = HazardMovementPattern::FollowTarget;

    followMaxSpeed = maxSpeed;
    followDetectionRange = detectionRange;
    followDistance = followDistanceValue;
    following = false;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);
}

bool MovingHazard::IsFollowing() const
{
    return following;
}

void MovingHazard::EnableRolling(float radius, RollAxisMode axisMode)
{
    rollingEnabled = true;
    rollingPivotHeight = std::max(radius, 0.0001f);
    rollingMotion.SetRadius(radius);
    rollingMotion.SetAxisMode(axisMode);
    rollingMotion.Reset();

    if (visual)
        rollingMotion.ApplyTo(visual->GetTransform());
}

void MovingHazard::Update(float deltaTime, const glm::vec3& targetGroundPosition)
{
    if (!visual || expired)
        return;

    switch (pattern)
    {
    case HazardMovementPattern::LinearSweep:
        UpdateLinearSweep(deltaTime);
        break;

    case HazardMovementPattern::CurvedSweep:
        UpdateCurvedSweep(deltaTime);
        break;

    case HazardMovementPattern::ArcProjectile:
        UpdateArcProjectile(deltaTime);
        break;

    case HazardMovementPattern::WarningThenStrike:
        UpdateWarningThenStrike(deltaTime);
        break;

    case HazardMovementPattern::TemporaryZone:
        UpdateTemporaryZone(deltaTime);
        break;

    case HazardMovementPattern::FollowTarget:
        UpdateFollowTarget(deltaTime, targetGroundPosition);
        break;
    }
}

void MovingHazard::UpdateLinearSweep(float deltaTime)
{
    const glm::vec3 to = sweepForward ? sweepEnd : sweepStart;

    const glm::vec3 currentPosition = visual->GetGroundPosition();
    const glm::vec3 toTarget = to - currentPosition;
    const float remaining = glm::length(toTarget);

    const float step = sweepSpeed * deltaTime;

    if (remaining <= step || remaining <= 0.0001f)
    {
        MoveVisualTo(to);
        velocity = glm::vec3(0.0f);

        if (sweepLoop)
        {
            // Turn around and sweep back the other way.
            sweepForward = !sweepForward;
        }
        else
        {
            expired = true;
        }

        return;
    }

    const glm::vec3 direction = toTarget / remaining;
    velocity = direction * sweepSpeed;

    MoveVisualTo(currentPosition + velocity * deltaTime);
}

void MovingHazard::UpdateCurvedSweep(float deltaTime)
{
    curveElapsed += deltaTime;

    const float t = glm::clamp(curveElapsed / curveDuration, 0.0f, 1.0f);

    const glm::vec3 previousPosition = visual->GetGroundPosition();

    // The straight-line point between start and end...
    glm::vec3 position = glm::mix(curveStart, curveEnd, t);

    // ...bowed sideways by a perpendicular offset that is zero at both
    // ends and peaks at the midpoint (t = 0.5). This is what makes the
    // path a genuine curve rather than a straight line, per the GDD's
    // "[spears/fireballs] travel in a curved trajectory."
    const glm::vec3 pathDirection = curveEnd - curveStart;
    glm::vec3 perpendicular(-pathDirection.z, 0.0f, pathDirection.x);

    if (glm::length(perpendicular) > 0.0001f)
        perpendicular = glm::normalize(perpendicular);

    const float bow = 4.0f * t * (1.0f - t);
    position += perpendicular * curveOffsetAmount * bow;

    MoveVisualTo(position);

    if (deltaTime > 0.0f)
        velocity = (position - previousPosition) / deltaTime;

    if (t >= 1.0f)
        expired = true;
}

void MovingHazard::UpdateArcProjectile(float deltaTime)
{
    arcElapsed += deltaTime;

    const float t = glm::clamp(arcElapsed / arcDuration, 0.0f, 1.0f);

    const glm::vec3 previousPosition = visual->GetGroundPosition();

    glm::vec3 position = glm::mix(arcStart, arcEnd, t);

    // Simple parabolic lift: 0 at both ends, arcHeight at the midpoint.
    position.y += 4.0f * arcHeight * t * (1.0f - t);

    MoveVisualTo(position);

    if (deltaTime > 0.0f)
        velocity = (position - previousPosition) / deltaTime;

    if (t >= 1.0f)
        expired = true;
}

void MovingHazard::UpdateWarningThenStrike(float deltaTime)
{
    phaseElapsed += deltaTime;
    velocity = glm::vec3(0.0f);

    if (active)
    {
        // Currently striking; check whether the strike window is over.
        if (phaseElapsed >= strikeDuration)
        {
            active = false;
            phaseElapsed = 0.0f;
        }
    }
    else
    {
        // Currently telegraphing; check whether it's time to strike.
        if (phaseElapsed >= warningDuration)
        {
            active = true;
            phaseElapsed = 0.0f;
        }
    }
}

void MovingHazard::UpdateTemporaryZone(float deltaTime)
{
    zoneElapsed += deltaTime;

    if (zoneElapsed >= zoneDuration)
        expired = true;
}

void MovingHazard::UpdateFollowTarget(
    float deltaTime,
    const glm::vec3& targetGroundPosition)
{
    const glm::vec3 currentPosition = visual->GetGroundPosition();

    // Chases across the ground plane only; height-following (if the sheep
    // needs to cross lanes of different surface height) is left to
    // whatever places it, the same way the pawn follows terrain itself.
    glm::vec3 toTarget = targetGroundPosition - currentPosition;
    toTarget.y = 0.0f;

    const float distance = glm::length(toTarget);

    // Detection gate: the sheep grazes until the player wanders within
    // range, and gives up once the player gets far enough away again. This
    // is what stops it beelining across the whole field from the moment it
    // spawns.
    if (distance > followDetectionRange)
    {
        following = false;
        velocity = glm::vec3(0.0f);
        return;
    }

    following = true;

    // Standoff: close the gap only while further than followDistance, and
    // stop once inside it, so the sheep trails the player at a distance
    // instead of overlapping. A small band below followDistance is left
    // alone rather than corrected, so it doesn't jitter forward-and-back on
    // the boundary while the player stands still.
    if (distance <= followDistance)
    {
        velocity = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 direction = toTarget / distance;

    // Never overshoot the standoff ring in a single step: clamp the move to
    // whatever distance remains between here and followDistance out from the
    // target.
    const float distanceToRing = distance - followDistance;
    const float step = std::min(followMaxSpeed * deltaTime, distanceToRing);

    velocity = direction * followMaxSpeed;

    MoveVisualTo(currentPosition + direction * step);
}

void MovingHazard::MoveVisualTo(const glm::vec3& groundPosition)
{
    if (!visual)
        return;

    const glm::vec3 travelDelta =
        groundPosition - visual->GetGroundPosition();

    visual->SetGroundPosition(groundPosition);

    if (rollingEnabled)
    {
        rollingMotion.Advance(travelDelta);
        rollingMotion.ApplyTo(visual->GetTransform());

        // GroundEntity models are authored with their base at local y = 0,
        // while a rolling rock/log is centred one radius above that point.
        // Compensate the translation after rotation so the centre stays at
        // ground + radius instead of orbiting around the ground contact.
        Transform3D& transform = visual->GetTransform();
        const glm::vec3 rotationDegrees = transform.GetRotation();

        glm::mat4 rotation(1.0f);
        rotation = glm::rotate(
            rotation,
            glm::radians(rotationDegrees.y),
            glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(
            rotation,
            glm::radians(rotationDegrees.x),
            glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(
            rotation,
            glm::radians(rotationDegrees.z),
            glm::vec3(0.0f, 0.0f, 1.0f));

        const glm::vec3 localPivot =
            glm::vec3(0.0f, rollingPivotHeight, 0.0f) *
            transform.GetScale();
        const glm::vec3 rotatedPivot =
            glm::vec3(rotation * glm::vec4(localPivot, 0.0f));

        transform.SetPosition(
            groundPosition + localPivot - rotatedPivot);
    }
}

GroundEntity& MovingHazard::GetVisual()
{
    return *visual;
}

const GroundEntity& MovingHazard::GetVisual() const
{
    return *visual;
}

const glm::vec3& MovingHazard::GetVelocity() const
{
    return velocity;
}

bool MovingHazard::IsActive() const
{
    return active;
}

bool MovingHazard::HasExpired() const
{
    return expired;
}

HazardMovementPattern MovingHazard::GetMovementPattern() const
{
    return pattern;
}
