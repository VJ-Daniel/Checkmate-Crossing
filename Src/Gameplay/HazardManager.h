#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <glm.hpp>

#include "GameConfig.h"
#include "MovingHazard.h"
#include "Obstacle.h"
#include "ObstacleMeshFactory.h"

/*
    ============================================================
    Checkmate Crossing - Hazard Manager
    Gameplay Programmer: Ayub
    Date: 2026

    Description:
    Owns every active moving hazard: spawns their visuals from the mesh
    library, advances their movement each frame, and drops the ones that
    have expired (one-shot projectiles that reached their target).

    This only owns MOVEMENT. Placement -- which lane spawns which hazard,
    on what timer, at what pace -- is level-design data, and belongs in
    Liyuu's level system feeding these Spawn* calls. The example spawns
    wired up in Game.cpp right now are placeholders standing in for that.

    Collision detection and damage are Kaung's; his system should iterate
    GetHazards() and read each MovingHazard's GetVisual() (for position/
    footprint), GetVelocity() and IsActive() to decide what happens on
    overlap. Nothing here reacts to the player at all except the cow's
    FollowTarget pattern, which only chases a position -- it doesn't know
    or care what a "hit" means.
    ============================================================
*/
class HazardManager
{
public:

    explicit HazardManager(ObstacleMeshLibrary& meshLibrary);

    /// Straight sweep: covers arrows, cannonballs, rolling rocks and
    /// rolling logs -- they differ only in these parameters (rolling
    /// hazards should sweep along Z, not X, per the GDD calling them
    /// "vertical").
    MovingHazard& SpawnLinearHazard(
        HazardType type,
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float speed,
        bool loop);

    /// A sweep that bows sideways partway across: used for spears and
    /// fireballs, per the GDD's "travel in a curved trajectory."
    MovingHazard& SpawnCurvedHazard(
        HazardType type,
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float speed,
        float curveOffset);

    /// Lobbed arc between two points. Not used by any hazard in the GDD's
    /// table right now, but kept available as a general-purpose spawn.
    MovingHazard& SpawnArcHazard(
        HazardType type,
        const glm::vec3& startGroundPosition,
        const glm::vec3& endGroundPosition,
        float duration,
        float arcHeight);

    /// Stationary telegraph-then-strike zone: used for lightning.
    MovingHazard& SpawnWarningHazard(
        const glm::vec3& groundPosition,
        float warningDuration,
        float strikeDuration);

    /// Stationary, always-dangerous zone that expires after duration.
    /// Used for the fire patch a fireball leaves on impact -- Update()
    /// spawns one of these automatically when a Fireball's CurvedSweep
    /// expires, so callers don't normally need to call this directly.
    MovingHazard& SpawnTemporaryZone(
        HazardType type,
        const glm::vec3& groundPosition,
        float duration);

    /// A sheep that grazes until the pawn comes within detectionRange, then
    /// follows while keeping followDistance from it (per the refinement
    /// task). Uses the stationary-prop Cow model (ObstacleType::Cow) rather
    /// than a Hazard model, since that's where the existing mesh for it
    /// already lives -- see the note in Obstacle.h about its enum placement.
    ///
    /// detectionRange/followDistance default from GameConfig; pass explicit
    /// values to make a particular sheep warier or clingier.
    MovingHazard& SpawnCow(
        const glm::vec3& startGroundPosition,
        float maxSpeed,
        float detectionRange = GameConfig::SheepDetectionRange,
        float followDistance = GameConfig::SheepFollowDistance);

    /// Removes up to `count` of the active hazards nearest to position.
    /// Used by the pawn's Bishop ability.
    ///
    /// NOTE(Ayub): the GDD's Bishop/Queen "breakable obstacles" are
    /// specifically Fencing and Palisade -- stationary props that don't
    /// have a placement system yet (Liyuu) or breakability rules yet
    /// (Kaung). This operates on active moving hazards instead, since
    /// that's what this class actually owns today. Redirect or extend
    /// this once those other systems exist.
    ///
    /// Returns how many were actually removed (may be less than count).
    int RemoveNearest(const glm::vec3& position, int count);

    /// Registers a hazard-spawning callback that fires once immediately
    /// and then repeats every interval seconds -- this is what actually
    /// keeps a lane's hazards coming instead of firing once at level
    /// start and never again. Typical use: a lambda that calls one of the
    /// Spawn* methods above with fixed parameters.
    void RegisterRepeatingSpawn(float interval, std::function<void()> spawnFn);

    /// Advances every active hazard, runs any due repeating spawns, and
    /// removes any hazards that have expired.
    void Update(float deltaTime, const glm::vec3& pawnGroundPosition);

    const std::vector<std::unique_ptr<MovingHazard>>& GetHazards() const;

    void Clear();

private:

    struct RepeatingSpawn
    {
        float interval = 1.0f;
        float timer = 0.0f;
        std::function<void()> spawnFn;
    };

    ObstacleMeshLibrary& meshLibrary;

    std::vector<std::unique_ptr<MovingHazard>> hazards;

    std::vector<RepeatingSpawn> repeatingSpawns;
};