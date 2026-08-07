#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

#include <glm.hpp>

#include "Collision.h"
#include "Pawn.h"
#include "MovingHazard.h"
#include "Obstacle.h"

/// Main collision system for hazards
class HazardCollision
{
public:
    explicit HazardCollision(Pawn& pawn);

    /// Check all hazards against the pawn
    void Update(
        const std::vector<std::unique_ptr<MovingHazard>>& movingHazards,
        const std::vector<std::shared_ptr<StaticObstacle>>& stationaryHazards,
        float deltaTime);

    /// Blocks the pawn against a set of plain solid volumes.
    ///
    /// For structures that are not hazards and carry no damage, slow or
    /// knockback - the checkpoint gate's walls and leaves. They only ever
    /// need to be solid, so they go through the same resolve as everything
    /// else rather than growing a parallel collision path of their own.
    ///
    /// Refreshes the pawn's own volume first, so it is safe to call either
    /// side of Update.
    void BlockAgainstBoxes(const std::vector<CollisionBox>& boxes);

    /// Check against moving hazards only
    void CheckMovingHazards(
        const std::vector<std::unique_ptr<MovingHazard>>& hazards);

    /// Check against stationary hazards only
    void CheckStationaryHazards(
        const std::vector<std::shared_ptr<StaticObstacle>>& hazards);

    /// Check if the pawn is in a danger zone (fire, lightning)
    ///
    /// Needs deltaTime because floor fire burns on its own once-per-second
    /// schedule rather than on the shared hit cooldown.
    void CheckAreaHazards(
        const std::vector<std::unique_ptr<MovingHazard>>& hazards,
        float deltaTime);

    /// Get the pawn's collision component for other checks
    CollisionComponent& GetPawnCollision() { return pawnCollision; }

    /// Set the damage cooldown duration
    void SetDamageCooldown(float seconds) { damageCooldown = seconds; }

    /// Get time until the pawn can take damage again
    float GetDamageCooldownRemaining() const { return damageCooldownRemaining; }

    /// Reset the cooldown (used after respawn)
    void ResetCooldown() { damageCooldownRemaining = 0.0f; }

    /// Callback for when the pawn takes damage
    std::function<void(float, const glm::vec3&)> onDamageTaken;

    /// Callback for when the pawn is knocked back
    std::function<void(const glm::vec3&, bool)> onKnockback;

    /// Callback for when the pawn is blocked
    /// Fired when an obstacle blocks the pawn. Receives the resolved
    /// position and the push that got it there, so the listener can cancel
    /// only the blocked axis and leave the pawn sliding along the surface.
    std::function<void(const glm::vec3&, const glm::vec3&)> onBlocked;

    /// Callback for when the pawn is slowed
    std::function<void(float, float, bool)> onSlowApplied;

private:
    Pawn* pawn;
    CollisionComponent pawnCollision;

    float damageCooldown = 0.5f;
    float damageCooldownRemaining = 0.0f;

    /// How long the pawn has been standing in each floor fire it is
    /// currently touching, keyed by the hazard itself.
    ///
    /// Per hazard rather than one global timer so two overlapping patches
    /// each keep their own schedule, and per contact rather than per fire so
    /// stepping out and back in starts the burn over instead of resuming
    /// mid-count. Entries for fires the pawn is no longer standing in are
    /// dropped every frame, which is what makes leaving reset it.
    std::unordered_map<const MovingHazard*, float> fireContactTimers;

    /// Applies one floor-fire tick if this patch is due, and advances or
    /// resets its timer. Returns whether the pawn is standing in it, so the
    /// caller can retire the timers of the ones they left.
    bool UpdateFloorFireContact(
        const MovingHazard& hazard,
        const glm::vec3& pawnPosition,
        float deltaTime);

    // Apply damage and/or knockback to the pawn (with cooldown check).
    bool ApplyDamage(
        float damage,
        const glm::vec3& direction,
        float knockback,
        bool isCow = false);

    // Check block movement
    /// Pushes the pawn clear of one obstacle by the minimum distance and
    /// reports the push, so the listener can cancel just that axis.
    void BlockAgainst(const CollisionComponent& obstacle);

    void BlockAgainst(const CollisionBox& obstacle);

    /// Rebuilds the pawn's own collision volume from its current position.
    void RefreshPawnVolume();
};
