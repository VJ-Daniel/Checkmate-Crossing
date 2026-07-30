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
    {
        visual->SetGroundPosition(groundPosition);

        // TEMPORARY: dim tint while telegraphing so the warning/strike
        // cycle is actually visible for testing before John's real VFX
        // exists. Safe to remove once real telegraph/strike visuals land.
        visual->SetColor(glm::vec4(0.6f, 0.55f, 0.25f, 1.0f));
    }
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

void MovingHazard::SetFollowTarget(float maxSpeed)
{
    pattern = HazardMovementPattern::FollowTarget;

    followMaxSpeed = maxSpeed;

    active = true;
    expired = false;
    velocity = glm::vec3(0.0f);
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
        visual->SetGroundPosition(to);
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

    visual->SetGroundPosition(currentPosition + velocity * deltaTime);
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

    visual->SetGroundPosition(position);

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

    visual->SetGroundPosition(position);

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

            // TEMPORARY, see SetWarningThenStrike.
            if (visual)
                visual->SetColor(glm::vec4(0.6f, 0.55f, 0.25f, 1.0f));
        }
    }
    else
    {
        // Currently telegraphing; check whether it's time to strike.
        if (phaseElapsed >= warningDuration)
        {
            active = true;
            phaseElapsed = 0.0f;

            // TEMPORARY, see SetWarningThenStrike.
            if (visual)
                visual->SetColor(glm::vec4(1.0f, 1.0f, 0.85f, 1.0f));
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

    // Chases across the ground plane only; height-following (if the cow
    // needs to cross lanes of different surface height) is left to
    // whatever places it, the same way the pawn follows terrain itself.
    glm::vec3 toTarget = targetGroundPosition - currentPosition;
    toTarget.y = 0.0f;

    const float distance = glm::length(toTarget);

    if (distance < 0.01f)
    {
        velocity = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 direction = toTarget / distance;
    const float step = std::min(followMaxSpeed * deltaTime, distance);

    velocity = direction * followMaxSpeed;

    visual->SetGroundPosition(currentPosition + direction * step);
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
