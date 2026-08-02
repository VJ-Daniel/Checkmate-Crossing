#pragma once

#include <memory>
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

    /// Check against moving hazards only
    void CheckMovingHazards(
        const std::vector<std::unique_ptr<MovingHazard>>& hazards);

    /// Check against stationary hazards only
    void CheckStationaryHazards(
        const std::vector<std::shared_ptr<StaticObstacle>>& hazards);

    /// Check if the pawn is in a danger zone (fire, lightning)
    void CheckAreaHazards(
        const std::vector<std::unique_ptr<MovingHazard>>& hazards);

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
    std::function<void(const glm::vec3&)> onBlocked;

    /// Callback for when the pawn is slowed
    std::function<void(float, float, bool)> onSlowApplied;

private:
    Pawn* pawn;
    CollisionComponent pawnCollision;

    float damageCooldown = 0.5f;
    float damageCooldownRemaining = 0.0f;

    // Apply damage to the pawn (with cooldown check)
    bool ApplyDamage(float damage, const glm::vec3& direction, float knockback);

    // Check block movement
    void HandleBlock(const glm::vec3& position, const glm::vec3& direction);
};