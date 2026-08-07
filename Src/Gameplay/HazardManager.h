#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <glm.hpp>

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

    /// Stationary telegraph-then-strike zone: used for lightning. Expires
    /// after its strike, so repeated lightning comes from a repeating spawn.
    MovingHazard& SpawnWarningHazard(
        const glm::vec3& groundPosition,
        float warningDuration,
        float strikeDuration,
        float catchRadius);

    /// Stationary, always-dangerous zone that expires after duration.
    /// Used for the fire patch a fireball leaves on impact -- Update()
    /// spawns one of these automatically when a Fireball's CurvedSweep
    /// expires, so callers don't normally need to call this directly.
    MovingHazard& SpawnTemporaryZone(
        HazardType type,
        const glm::vec3& groundPosition,
        float duration);

    /// A cow that continuously chases the pawn. Uses the stationary-prop
    /// Cow model (ObstacleType::Cow) rather than a Hazard model, since
    /// that's where the existing mesh for it already lives -- see the
    /// note in Obstacle.h about its enum placement.
    MovingHazard& SpawnCow(
        const glm::vec3& startGroundPosition,
        float maxSpeed);

    /// Removes up to `maxCount` of the stationary props nearest to origin,
    /// ignoring any that lie further away than radius. Used by the pawn's
    /// Bishop ability, and through it the Queen's.
    ///
    /// Only types IsAbilityClearable() accepts are eligible, so the cow is
    /// skipped along with every moving hazard. Nothing this class owns is
    /// touched: the ability is explicitly not allowed to delete projectiles
    /// (see the rule in Obstacle.h).
    ///
    /// This used to remove the nearest MOVING hazards instead, which was
    /// always a stand-in - the GDD's Bishop breaks "breakable obstacles",
    /// and the original note here asked for exactly this redirect once the
    /// props had a real placement system. They do now (Game owns them), so
    /// the obstacles are passed in rather than being reached for: this
    /// class still owns only movement.
    ///
    /// Cleared ground positions are appended to clearedPositions so the
    /// caller can show the player what was destroyed without this function
    /// knowing anything about rendering.
    ///
    /// Returns how many were actually removed (may be fewer than maxCount).
    static int ClearStationaryObstacles(
        std::vector<std::shared_ptr<StaticObstacle>>& obstacles,
        const glm::vec3& origin,
        float radius,
        int maxCount,
        std::vector<glm::vec3>& clearedPositions,
        std::vector<std::shared_ptr<GroundEntity>>& removedVisuals);

    /// The moving-hazard half of the same ability.
    ///
    /// Only hazards IsAbilityClearable(HazardType) accepts are eligible,
    /// which today means the cow and nothing else - every projectile is
    /// protected, in flight or not. Without this the cow would be immune
    /// simply because it happens to be driven by a MovingHazard rather than
    /// standing in the obstacle list.
    ///
    /// Same reporting contract as ClearStationaryObstacles: positions for
    /// the effect, visuals for the caller to play a death reaction on.
    int ClearRemovableHazards(
        const glm::vec3& origin,
        float radius,
        int maxCount,
        std::vector<glm::vec3>& clearedPositions,
        std::vector<std::shared_ptr<GroundEntity>>& removedVisuals);

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