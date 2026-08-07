/*
    ============================================================
    Checkmate Crossing - Moving Hazard
    Gameplay Programmer: Ayub
    Date: 2026

    See MovingHazard.h for an overview of each movement pattern.
    ============================================================
*/

#include "MovingHazard.h"

#include <cmath>

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
    float strikeDurationValue,
    float catchRadiusValue)
{
    pattern = HazardMovementPattern::WarningThenStrike;

    warningDuration = warningDurationValue;
    strikeDuration = strikeDurationValue;
    catchRadius = catchRadiusValue;
    phaseElapsed = 0.0f;

    lightningPhase = LightningPhase::Warning;
    caughtTarget = false;
    strikeHitPending = false;

    // Until the strike resolves, the marker's own centre is the best answer
    // to "where will this land", and it is the answer used verbatim if the
    // player gets clear in time.
    strikePosition = groundPosition;

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

void MovingHazard::SetFollowLimitZ(float minimumZ)
{
    followMinZ = minimumZ;
}

void MovingHazard::SetFacesTravel(bool shouldFaceTravel)
{
    facesTravel = shouldFaceTravel;
}

bool MovingHazard::GetFacesTravel() const
{
    return facesTravel;
}

void MovingHazard::UpdateFacing(float deltaTime)
{
    if (!facesTravel || !visual || deltaTime <= 0.0f)
        return;

    // Below this the direction is numerical noise rather than travel, and
    // turning toward it is exactly what makes a near-stationary hazard
    // jitter. Hold the last heading instead.
    constexpr float MinimumSpeed = 0.05f;

    // Degrees per second. Fast enough that a hazard reversing at the end of
    // its sweep has turned round before it is back on screen, slow enough
    // that the turn reads as a turn rather than a snap.
    constexpr float TurnDegreesPerSecond = 540.0f;

    const glm::vec3 travel(velocity.x, 0.0f, velocity.z);

    if (glm::length(travel) < MinimumSpeed)
        return;

    // Every hazard model with a nose is authored pointing +X, and a Y
    // rotation of theta carries +X round to (cos theta, 0, -sin theta).
    // Aiming that at the travel direction is therefore atan2(-z, x).
    const float target =
        glm::degrees(std::atan2(-travel.z, travel.x));

    if (!facingInitialised)
    {
        facingDegrees = target;
        facingInitialised = true;
    }
    else
    {
        // Shortest way round, so a heading crossing the +/-180 seam turns
        // the short way instead of spinning all the way back through zero.
        float delta = target - facingDegrees;

        while (delta > 180.0f)
            delta -= 360.0f;

        while (delta < -180.0f)
            delta += 360.0f;

        const float maxStep = TurnDegreesPerSecond * deltaTime;

        facingDegrees += glm::clamp(delta, -maxStep, maxStep);
    }

    visual->GetTransform().SetRotation(0.0f, facingDegrees, 0.0f);
}

void MovingHazard::EnableRolling(float radius, RollAxisMode axisMode)
{
    rollingEnabled = true;

    // A rolling rock or log writes its own rotation every frame, so the
    // facing pass has to stand down or the two fight over the transform.
    facesTravel = false;
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
        // Needs the target for the same reason FollowTarget does: it has to
        // know where the player is at the instant the countdown ends.
        UpdateWarningThenStrike(deltaTime, targetGroundPosition);
        break;

    case HazardMovementPattern::TemporaryZone:
        UpdateTemporaryZone(deltaTime);
        break;

    case HazardMovementPattern::FollowTarget:
        UpdateFollowTarget(deltaTime, targetGroundPosition);
        break;
    }

    // Last, so it reads the velocity the pattern above has just settled on.
    // One pass for every pattern rather than one per pattern: the rule is
    // the same whichever way the hazard decided to move.
    UpdateFacing(deltaTime);
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

void MovingHazard::UpdateWarningThenStrike(
    float deltaTime,
    const glm::vec3& targetGroundPosition)
{
    if (lightningPhase == LightningPhase::Finished)
        return;

    phaseElapsed += deltaTime;
    velocity = glm::vec3(0.0f);

    if (lightningPhase == LightningPhase::Warning)
    {
        if (phaseElapsed < warningDuration)
            return;

        // The countdown has just hit zero. Everything about this strike is
        // decided here, in this one frame, and never revisited:
        //
        //   - was the player inside the marked area?
        //   - if so, exactly where were they standing?
        //
        // Sampling once is what makes the rule honest in both directions.
        // A player who left in time cannot be dragged back by a bolt that
        // re-checks later, and a player who stayed cannot dodge it by
        // sprinting out during the strike animation.
        const glm::vec3 offset = targetGroundPosition - groundPositionOfVisual();

        const float distance = glm::length(
            glm::vec3(offset.x, 0.0f, offset.z));

        caughtTarget = distance <= catchRadius;

        // Caught: the bolt comes down on them, wherever that was. Escaped:
        // it falls on the marker, harmlessly, so the telegraph still pays
        // off visually instead of just evaporating.
        strikePosition = caughtTarget
            ? targetGroundPosition
            : groundPositionOfVisual();

        strikeHitPending = caughtTarget;

        lightningPhase = LightningPhase::Strike;
        phaseElapsed = 0.0f;

        // Only a strike that actually caught someone is dangerous. A missed
        // strike stays inactive so no collision pass can find damage in it.
        active = caughtTarget;

        return;
    }

    // Striking. The visual plays out for its full window either way; only
    // the damage was conditional.
    if (phaseElapsed >= strikeDuration)
    {
        lightningPhase = LightningPhase::Finished;
        active = false;
        expired = true;
    }
}

const glm::vec3& MovingHazard::groundPositionOfVisual() const
{
    static const glm::vec3 origin = glm::vec3(0.0f);

    return visual ? visual->GetGroundPosition() : origin;
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

    glm::vec3 next = currentPosition + direction * step;

    // Leash. Rows run toward -Z, so this is the furthest along the level the
    // hazard may chase. Without it a follower simply walks into whatever
    // comes next -- a cow tailing the player into a section that is supposed
    // to hold nothing but fireballs and lightning.
    if (next.z < followMinZ)
    {
        next.z = followMinZ;

        // Report what it is actually doing, so a leashed follower pressed
        // against its limit does not read as still charging forward.
        velocity.z = 0.0f;
    }

    MoveVisualTo(next);
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

const std::shared_ptr<GroundEntity>& MovingHazard::GetVisualShared() const
{
    return visual;
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

float MovingHazard::GetPhaseElapsed() const
{
    return phaseElapsed;
}

float MovingHazard::GetWarningDuration() const
{
    return warningDuration;
}

float MovingHazard::GetStrikeDuration() const
{
    return strikeDuration;
}

float MovingHazard::GetCurveElapsed() const
{
    return curveElapsed;
}

float MovingHazard::GetCurveDuration() const
{
    return curveDuration;
}

LightningPhase MovingHazard::GetLightningPhase() const
{
    return lightningPhase;
}

float MovingHazard::GetCatchRadius() const
{
    return catchRadius;
}

const glm::vec3& MovingHazard::GetStrikePosition() const
{
    return strikePosition;
}

bool MovingHazard::DidCatchTarget() const
{
    return caughtTarget;
}

bool MovingHazard::ConsumeStrikeHit()
{
    if (!strikeHitPending)
        return false;

    strikeHitPending = false;
    return true;
}

float MovingHazard::GetArcElapsed() const
{
    return arcElapsed;
}

float MovingHazard::GetArcDuration() const
{
    return arcDuration;
}

float MovingHazard::GetArcGroundHeight() const
{
    return arcStart.y;
}

float MovingHazard::GetZoneElapsed() const
{
    return zoneElapsed;
}

float MovingHazard::GetZoneDuration() const
{
    return zoneDuration;
}
