#pragma once

#include <memory>

#include <glm.hpp>

#include "GroundEntity.h"
#include "RollingMotion.h"

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

/// Which stage of its life a WarningThenStrike hazard is in.
///
/// Only lightning uses this. The pattern used to cycle warning and strike
/// forever, which meant it had no moment of resolution to hang a decision on
/// - and the whole point of lightning is that one instant when the countdown
/// hits zero and the player either got clear or did not.
enum class LightningPhase
{
    /// Telegraphing. The marker is on the ground and nothing is dangerous.
    Warning,

    /// The bolt is down. Whether it caught anyone was decided at the moment
    /// this phase began, not continuously.
    Strike,

    /// Done. The hazard has expired and its owner should drop it.
    Finished
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

    /// Stays at groundPosition and telegraphs for warningDuration, then
    /// strikes for strikeDuration and expires.
    ///
    /// catchRadius is the marked area. At the instant the warning ends, the
    /// target passed to Update is tested against it once: inside means the
    /// strike lands on the target's position at that moment and DidCatchTarget
    /// reports true; outside means the strike falls harmlessly on the marker's
    /// own centre and nothing is damaged. Moving away afterwards cannot undo
    /// a hit, and staying still afterwards cannot cause one - the decision
    /// belongs to that single frame.
    ///
    /// This used to loop warning and strike forever. It now finishes, so
    /// repeated lightning comes from a repeating spawn like every other
    /// hazard rather than from one immortal object.
    void SetWarningThenStrike(
        const glm::vec3& groundPosition,
        float warningDuration,
        float strikeDuration,
        float catchRadius);

    /// Stays at groundPosition and is active for duration, then expires.
    /// Used for the fireball's residual fire patch.
    void SetTemporaryZone(
        const glm::vec3& groundPosition,
        float duration);

    /// Chases a target with simple "sheep" AI (per the refinement task):
    ///
    ///   - It ignores the target until the target comes within
    ///     detectionRange. Before that it stands still, and once the
    ///     target leaves that range again it stops and waits.
    ///   - Once following, it holds followDistance from the target rather
    ///     than sticking to it: it closes the gap only while further away
    ///     than that, and stops once it's within it.
    ///
    /// maxSpeed caps how fast it moves while closing. The visual's current
    /// ground position (set by the caller beforehand, e.g. via
    /// CreateObstacle(...)->SetGroundPosition(...)) is where it starts from.
    ///
    /// detectionRange <= followDistance would mean "notice and stop at the
    /// same time" (it would never take a step), so callers should keep
    /// detectionRange comfortably larger than followDistance.
    void SetFollowTarget(
        float maxSpeed,
        float detectionRange,
        float followDistance);

    /// True once the target has come within detection range and the sheep
    /// has begun following. Exposed so animation (John) or collision
    /// (Kaung) can tell an alert, moving sheep from an idle grazing one.
    bool IsFollowing() const;

    /// Furthest along the level a follower may chase, as a world Z.
    ///
    /// Rows run toward -Z, so this is a floor on Z rather than a ceiling. It
    /// exists because a follower otherwise has no idea which section it
    /// belongs to: it will happily tail the player out of its own woodland
    /// and into the finale, which is meant to hold nothing but fireballs and
    /// lightning. Defaults to no limit.
    void SetFollowLimitZ(float minimumZ);

    /// Connects the visual-only rolling component to this mover. Translation
    /// remains owned here; RollingMotion derives spin from the actual distance
    /// travelled so speed changes and reversals stay visually correct.
    //---------------------------------------------------------
    // Facing
    //---------------------------------------------------------

    /// Turns the visual to face the way it is travelling.
    ///
    /// On by default. Every hazard model with a nose - the arrow, the spear,
    /// the cow - is authored pointing +X, so one rule serves all of them.
    ///
    /// This is what stops the cow permanently facing whichever way it was
    /// placed while it chases the player around, and what turns a looping
    /// arrow round on its return leg instead of flying it tail-first.
    ///
    /// Automatically disabled by EnableRolling: a rolling rock or log
    /// already owns its rotation, and two systems writing the same transform
    /// would fight every frame.
    void SetFacesTravel(bool facesTravel);

    bool GetFacesTravel() const;

    void EnableRolling(float radius, RollAxisMode axisMode);

    //-----------------------------------------------------------

    /// Advances the pattern. targetGroundPosition is only read by
    /// FollowTarget; every other pattern ignores it, so it's safe to
    /// always pass the pawn's ground position here regardless of pattern.
    void Update(float deltaTime, const glm::vec3& targetGroundPosition);

    GroundEntity& GetVisual();

    const GroundEntity& GetVisual() const;

    /// Shared ownership of the visual, for the rare caller that needs it to
    /// outlive the hazard - a removed cow still has a death animation to
    /// finish after this object is gone.
    const std::shared_ptr<GroundEntity>& GetVisualShared() const;

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

    HazardMovementPattern GetMovementPattern() const;

    /// Current time inside the warning or strike phase. Only meaningful for
    /// WarningThenStrike hazards such as lightning.
    float GetPhaseElapsed() const;

    float GetWarningDuration() const;

    float GetStrikeDuration() const;

    //-----------------------------------------------------------
    // Lightning resolution
    //
    // All four are only meaningful for WarningThenStrike.
    //-----------------------------------------------------------

    LightningPhase GetLightningPhase() const;

    /// Radius of the marked area.
    float GetCatchRadius() const;

    /// Where the bolt actually comes down: the target's position at the
    /// instant the countdown hit zero if it was caught, otherwise the
    /// marker's own centre.
    ///
    /// Fixed once the strike begins. The effect must not follow the target
    /// after the fact, so this deliberately stops tracking.
    const glm::vec3& GetStrikePosition() const;

    /// Whether the strike caught the target. False for the whole warning
    /// phase, and false forever after a strike that missed.
    bool DidCatchTarget() const;

    /// True exactly once, on the frame a strike lands on the target.
    ///
    /// Consumed rather than polled so damage is applied a single time no
    /// matter how many frames the strike visual lasts, which is what keeps
    /// one bolt to one hit without the collision system tracking that
    /// itself.
    bool ConsumeStrikeHit();

    /// Progress for curved projectiles such as spears.
    float GetCurveElapsed() const;

    float GetCurveDuration() const;

    /// Progress for lobbed projectiles such as fireballs. Lets an effect
    /// pick an animation frame from how far through its flight the
    /// projectile is, without duplicating the trajectory maths.
    float GetArcElapsed() const;

    float GetArcDuration() const;

    /// Surface height the lob was launched from and lands back on.
    ///
    /// The visual's own position carries the parabola, so it is no use for
    /// answering "where is the ground under this". A shadow needs that
    /// separately, and subtracting the two is also how far up the projectile
    /// currently is.
    float GetArcGroundHeight() const;

    /// Where the lob is aimed to come down.
    ///
    /// Known from the moment it is launched, which is what lets an effect
    /// mark the landing spot while the projectile is still in the air.
    const glm::vec3& GetArcLandingPosition() const;

    /// Progress for temporary zones such as the fireball's lingering impact.
    float GetZoneElapsed() const;

    float GetZoneDuration() const;

private:

    void UpdateLinearSweep(float deltaTime);

    void UpdateCurvedSweep(float deltaTime);

    void UpdateArcProjectile(float deltaTime);

    void UpdateWarningThenStrike(
        float deltaTime,
        const glm::vec3& targetGroundPosition);

    void UpdateTemporaryZone(float deltaTime);

    void UpdateFollowTarget(float deltaTime, const glm::vec3& targetGroundPosition);

    void MoveVisualTo(const glm::vec3& groundPosition);

    /// The visual's ground position, or the origin if it has none. Saves
    /// repeating the null check inside the strike resolution.
    const glm::vec3& groundPositionOfVisual() const;

    std::shared_ptr<GroundEntity> visual;

    HazardMovementPattern pattern = HazardMovementPattern::LinearSweep;

    glm::vec3 velocity = glm::vec3(0.0f);

    bool active = true;

    bool expired = false;

    /// Turns the visual toward its current velocity at a fixed rate.
    void UpdateFacing(float deltaTime);

    /// Whether the visual turns to match its travel direction.
    bool facesTravel = true;

    /// Current smoothed heading in degrees, and whether it has been set at
    /// least once. The first heading is snapped rather than turned toward,
    /// so a hazard does not visibly swing round on the frame it spawns.
    float facingDegrees = 0.0f;

    bool facingInitialised = false;

    bool rollingEnabled = false;

    RollingMotion rollingMotion;

    /// Local-space height of the model's rolling centre above its ground
    /// contact. Used to keep that centre fixed while the mesh rotates.
    float rollingPivotHeight = 0.0f;

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

    LightningPhase lightningPhase = LightningPhase::Warning;

    float catchRadius = 1.0f;

    glm::vec3 strikePosition = glm::vec3(0.0f);

    bool caughtTarget = false;

    bool strikeHitPending = false;

    // --- TemporaryZone ---

    float zoneDuration = 1.0f;
    float zoneElapsed = 0.0f;

    // --- FollowTarget ---

    float followMaxSpeed = 2.0f;

    /// How close the target must come before the sheep starts following.
    float followDetectionRange = 5.0f;

    /// The gap the sheep tries to keep once following, so it trails the
    /// target rather than overlapping it.
    float followDistance = 1.5f;

    /// Latched true the first time the target enters detection range, and
    /// back to false whenever the target is outside it again.
    bool following = false;

    /// Furthest along the level this follower may chase. Effectively no
    /// limit until a caller sets one.
    float followMinZ = -1.0e9f;
};
