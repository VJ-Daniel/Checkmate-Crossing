#pragma once

#include <memory>

#include <glm.hpp>

#include "GroundEntity.h"

/*
    ============================================================
    Checkmate Crossing - Moving Hazard
    Gameplay Programmer: Ayub
    Date: 2026

    Description:
    Drives a hazard's movement every frame according to one of several
    reusable patterns, wrapping whatever GroundEntity visual it was
    given (a Hazard model from ObstacleMeshLibrary::CreateHazard, or --
    for the cow -- a StaticObstacle from CreateObstacle(ObstacleType::Cow)).

    This class owns movement ONLY. It never detects collisions, applies
    damage, or plays animations. It exposes GetVelocity(), IsActive() and
    HasExpired() so Kaung's collision system and John's animation/VFX
    system have everything they need to react on their end, matching the
    team's "Ayub owns how things move, Kaung owns what happens when they
    collide" split.
    ============================================================
*/

/// How a moving hazard's position evolves over time.
enum class HazardMovementPattern
{
    /// A straight sweep between two ground points. Covers arrows,
    /// cannonballs, rolling rocks and rolling logs -- they differ only in
    /// speed, spacing, axis (rolling hazards sweep along Z, not X) and
    /// whether they loop back and forth.
    LinearSweep,

    /// A sweep between two ground points that bows sideways partway
    /// across, per the GDD: "[spears] travel in a curved trajectory."
    /// Fireballs use this too -- the GDD calls them "similar to spears."
    CurvedSweep,

    /// A parabolic lob between two ground points, peaking partway across.
    /// Not currently used by any hazard in the GDD's table (nothing there
    /// describes a vertical lob), but kept available as a general-purpose
    /// pattern.
    ArcProjectile,

    /// Stays in place. Telegraphs for a warning duration (IsActive() ==
    /// false), then is dangerous for a strike duration (IsActive() ==
    /// true), then repeats forever. Used for lightning.
    WarningThenStrike,

    /// Stays in place and is active for a fixed duration, then expires.
    /// Used for the fire patch a fireball leaves behind on impact.
    TemporaryZone,

    /// Continuously closes in on whatever position Update() is given,
    /// capped to a maximum speed. Used for the cow.
    FollowTarget
};

/// Moves one hazard's visual according to its configured pattern.
class MovingHazard
{
public:

    explicit MovingHazard(std::shared_ptr<GroundEntity> visual);

    //-----------------------------------------------------------
    // Pattern configuration. Call exactly one of these once, right
    // after construction, before the first Update.
    //-----------------------------------------------------------

    /// Sweeps between two ground points at a constant speed. If loop is
    /// true it bounces back and forth forever; otherwise HasExpired()
    /// becomes true once it reaches the end point, so its owner can
    /// remove it.
    void SetLinearSweep(
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float speed,
        bool loop);

    /// Sweeps between two ground points like SetLinearSweep, but bows
    /// sideways by curveOffset at the midpoint instead of going straight
    /// -- a genuine curved path, not just a straight line. Speed is
    /// measured along the straight-line distance, so it still means
    /// roughly what it says despite the detour. Expires on arrival.
    void SetCurvedSweep(
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float speed,
        float curveOffset);

    /// Lobs from start to end over the given duration, peaking arcHeight
    /// above the straight line between them. Expires on arrival.
    void SetArcProjectile(
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float duration,
        float arcHeight);

    /// Stays at groundPosition. Cycles a warning phase (not active) and a
    /// strike phase (active) forever; never expires on its own.
    void SetWarningThenStrike(
        const glm::vec3& groundPosition,
        float warningDuration,
        float strikeDuration);

    /// Stays at groundPosition and is active for duration, then expires.
    /// Used for the fireball's residual fire patch.
    void SetTemporaryZone(
        const glm::vec3& groundPosition,
        float duration);

    /// Chases whatever position Update() is given, capped to maxSpeed.
    /// The visual's current ground position (set by the caller beforehand,
    /// e.g. via CreateObstacle(...)->SetGroundPosition(...)) is where it
    /// starts from.
    void SetFollowTarget(float maxSpeed);

    //-----------------------------------------------------------

    /// Advances the pattern. targetGroundPosition is only read by
    /// FollowTarget; every other pattern ignores it, so it's safe to
    /// always pass the pawn's ground position here regardless of pattern.
    void Update(float deltaTime, const glm::vec3& targetGroundPosition);

    GroundEntity& GetVisual();

    const GroundEntity& GetVisual() const;

    /// World-space velocity this frame. Zero while a WarningThenStrike
    /// hazard is standing still (which is always, for that pattern).
    const glm::vec3& GetVelocity() const;

    /// True while this hazard should actually cause damage/effects right
    /// now. Always true except during a WarningThenStrike's telegraph
    /// phase.
    bool IsActive() const;

    /// True once a one-shot pattern (non-looping LinearSweep, any
    /// CurvedSweep, any ArcProjectile, or an expired TemporaryZone) has
    /// finished; the owner should remove it.
    bool HasExpired() const;

private:

    void UpdateLinearSweep(float deltaTime);

    void UpdateCurvedSweep(float deltaTime);

    void UpdateArcProjectile(float deltaTime);

    void UpdateWarningThenStrike(float deltaTime);

    void UpdateTemporaryZone(float deltaTime);

    void UpdateFollowTarget(float deltaTime, const glm::vec3& targetGroundPosition);

    std::shared_ptr<GroundEntity> visual;

    HazardMovementPattern pattern = HazardMovementPattern::LinearSweep;

    glm::vec3 velocity = glm::vec3(0.0f);

    bool active = true;

    bool expired = false;

    // --- LinearSweep ---

    glm::vec3 sweepStart = glm::vec3(0.0f);
    glm::vec3 sweepEnd = glm::vec3(0.0f);
    float sweepSpeed = 0.0f;
    bool sweepLoop = false;
    bool sweepForward = true;

    // --- CurvedSweep ---

    glm::vec3 curveStart = glm::vec3(0.0f);
    glm::vec3 curveEnd = glm::vec3(0.0f);
    float curveOffsetAmount = 0.0f;
    float curveDuration = 1.0f;
    float curveElapsed = 0.0f;

    // --- ArcProjectile ---

    glm::vec3 arcStart = glm::vec3(0.0f);
    glm::vec3 arcEnd = glm::vec3(0.0f);
    float arcDuration = 1.0f;
    float arcHeight = 1.0f;
    float arcElapsed = 0.0f;

    // --- WarningThenStrike ---

    float warningDuration = 1.0f;
    float strikeDuration = 1.0f;
    float phaseElapsed = 0.0f;

    // --- TemporaryZone ---

    float zoneDuration = 1.0f;
    float zoneElapsed = 0.0f;

    // --- FollowTarget ---

    float followMaxSpeed = 2.0f;
};
