#include "HazardCollision.h"
#include "GameConfig.h"
#include <algorithm>
#include <cmath>

// ---- Helper to get collision shape from hazards ----

namespace
{
    /// Get a collision component from a MovingHazard's visual
    CollisionComponent GetHazardCollision(const MovingHazard& hazard, float scale = 0.8f)
    {
        const GroundEntity& visual = hazard.GetVisual();
        CollisionComponent comp;
        comp.UpdateFromEntity(visual, scale);

        // Override for specific hazard types with different shapes
        const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);
        if (hazardVisual)
        {
            HazardType type = hazardVisual->GetType();

            if (type == HazardType::Arrow || type == HazardType::Spear)
            {
                // Arrow/spear are long and thin - use a smaller box
                glm::vec3 position = visual.GetGroundPosition();
                float height = visual.GetHeight();
                float width = visual.GetFootprintWidth();
                float depth = visual.GetFootprintDepth();

                glm::vec3 center = position + glm::vec3(0.0f, height * 0.5f, 0.0f);
                comp.SetBox(center, glm::vec3(width * 0.3f, height * 0.3f, depth * 0.3f));
            }
            else if (type == HazardType::Cannonball)
            {
                // Cannonball is round
                float radius = visual.GetFootprintWidth() * 0.5f * 0.8f;
                glm::vec3 center = visual.GetGroundPosition() + glm::vec3(0.0f, visual.GetHeight() * 0.5f, 0.0f);
                comp.SetCircle(center, radius);
            }
            else if (type == HazardType::RollingRock)
            {
                // Rolling Rock is a large sphere. We use a Circle collision slightly larger than the visual.
                float radius = visual.GetFootprintWidth() * 0.7f;
                if (radius <= 0.0f) radius = 0.28f * 0.7f; // Fallback to the model radius if width is 0

                glm::vec3 center = visual.GetGroundPosition() + glm::vec3(0.0f, visual.GetHeight() * 0.5f, 0.0f);
                comp.SetCircle(center, radius);
            }
            else if (type == HazardType::RollingLog)
            {
                // Rolling Log is a cylinder. We use a Box.
                glm::vec3 position = visual.GetGroundPosition();
                float height = visual.GetHeight();
                float width = visual.GetFootprintWidth();
                float depth = visual.GetFootprintDepth();

                // If depth is 0, default to the radius logic
                if (depth <= 0.0f) depth = width * 0.25f;

                glm::vec3 center = position + glm::vec3(0.0f, height * 0.5f, 0.0f);
                comp.SetBox(center, glm::vec3(width * 0.45f, height * 0.45f, depth * 0.45f));
            }
        }

        return comp;
    }

    /// Get a collision component from a StaticObstacle
    CollisionComponent GetObstacleCollision(const StaticObstacle& obstacle, float scale = 0.85f)
    {
        CollisionComponent comp;

        // Manually compute the exact center of the box
        const glm::vec3 position = obstacle.GetGroundPosition();
        const float height = obstacle.GetHeight();
        const float width = obstacle.GetFootprintWidth() * scale;
        const float depth = obstacle.GetFootprintDepth() * scale;

        // Center of the object (halfway up from the ground)
        const glm::vec3 center = position + glm::vec3(0.0f, height * 0.5f, 0.0f);

        // Special cases based on obstacle type
        ObstacleType type = obstacle.GetType();

        if (type == ObstacleType::Spikes)
        {
            // Spikes are smaller than they look (the visual has a base plate)
            const float scaledWidth = width * 0.5f;
            const float scaledDepth = depth * 0.5f;
            comp.SetBox(center, glm::vec3(scaledWidth, height * 0.5f, scaledDepth));
        }
        else if (type == ObstacleType::Bush || type == ObstacleType::Mud)
        {
            // Force exact collision size for Mud and Bush
            if (type == ObstacleType::Mud)
            {
                // Mud is 0.8 wide and 0.3 tall. 
                comp.SetBox(center, glm::vec3(0.4f, 0.15f, 0.4f));
            }
            else if (type == ObstacleType::Bush)
            {
                // Bush is 0.66 wide and 0.48 tall.
                comp.SetBox(center, glm::vec3(0.33f, 0.24f, 0.33f));
            }
        }
        else
        {
            // Default for Walls, Fences, Trees, Rocks, Palisades
            comp.SetBox(center, glm::vec3(width * 0.5f, height * 0.5f, depth * 0.5f));
        }

        return comp;
    }
}

// ---- HazardCollision ----

HazardCollision::HazardCollision(Pawn& pawn)
    : pawn(&pawn)
{
    float pawnWidth = 0.4f;
    float pawnHeight = 0.6f;

    glm::vec3 center = pawn.GetTransform().GetPosition();

    pawnCollision.SetBox(center, glm::vec3(pawnWidth * 0.4f, pawnHeight * 0.4f, pawnWidth * 0.4f));
}

void HazardCollision::Update(
    const std::vector<std::unique_ptr<MovingHazard>>& movingHazards,
    const std::vector<std::shared_ptr<StaticObstacle>>& stationaryHazards,
    float deltaTime)
{
    if (!pawn)
        return;

    // Recalculate the pawn's collision box size every frame
    float pawnWidth = 0.4f;
    float pawnHeight = 0.6f;

    pawnCollision.SetBox(
        pawn->GetTransform().GetPosition(),
        glm::vec3(pawnWidth * 0.4f, pawnHeight * 0.4f, pawnWidth * 0.4f));

    // Update cooldown
    if (damageCooldownRemaining > 0.0f)
        damageCooldownRemaining -= deltaTime;

    // Check moving hazards
    CheckMovingHazards(movingHazards);

    // Check stationary hazards
    CheckStationaryHazards(stationaryHazards);

    // Check area hazards (fire patches, lightning strikes)
    CheckAreaHazards(movingHazards);
}

void HazardCollision::CheckMovingHazards(
    const std::vector<std::unique_ptr<MovingHazard>>& hazards)
{
    if (!pawn)
        return;

    for (const auto& hazard : hazards)
    {
        if (!hazard || hazard->HasExpired() || !hazard->IsActive())
            continue;

        const GroundEntity& visual = hazard->GetVisual();

        // Skip if the pawn is immune
        if (pawn->IsImmuneToHazards())
            continue;

        CollisionComponent hazardCollision = GetHazardCollision(*hazard);

        if (pawnCollision.Intersects(hazardCollision))
        {
            const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);
            HazardType type = hazardVisual ? hazardVisual->GetType() : HazardType::Arrow;

            // Determine damage and effects based on hazard type
            float damage = 1.0f;
            float knockback = 1.5f;
            glm::vec3 knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);

            // Get the hazard's velocity for knockback direction
            glm::vec3 velocity = hazard->GetVelocity();
            if (glm::length(velocity) > 0.01f)
                knockbackDir = glm::normalize(velocity);

            // If shield is available, consume it instead of taking damage
            if (pawn->HasShield())
            {
                pawn->ConsumeShield();
                // Still apply knockback, but no damage
                ApplyDamage(0.0f, knockbackDir, knockback * 0.5f);
                continue;
            }

            // Apply damage and effects
            ApplyDamage(damage, knockbackDir, knockback);

            // Special effects for specific hazard types
            if (type == HazardType::Fireball)
            {
                ApplyDamage(1.0f, knockbackDir, 2.0f);
            }
            else if (type == HazardType::Cannonball)
            {
                ApplyDamage(0.5f, knockbackDir, 2.5f);
            }
            else if (type == HazardType::RollingRock)
            {
                // Rolling Rock: "large hit area, easier to see, harder to move around"
                ApplyDamage(1.0f, knockbackDir, 2.0f); // High damage, strong push
            }
            else if (type == HazardType::RollingLog)
            {
                // Rolling Log: Similar damage, slightly lighter
                ApplyDamage(0.5f, knockbackDir, 2.5f);
            }
        }
    }
}

void HazardCollision::CheckStationaryHazards(
    const std::vector<std::shared_ptr<StaticObstacle>>& hazards)
{
    if (!pawn)
        return;

    for (const auto& obstacle : hazards)
    {
        if (!obstacle)
            continue;

        CollisionComponent obstacleCollision = GetObstacleCollision(*obstacle);

        if (pawnCollision.Intersects(obstacleCollision))
        {
            ObstacleType type = obstacle->GetType();
            glm::vec3 pawnVelocity = pawn->GetVelocity();
            bool isJumping = (pawnVelocity.y > 0.1f);

            // ==========================================
            // 1. WALLS & TREES: Completely Block (Cannot be jumped over)
            // ==========================================
            if (type == ObstacleType::Wall || type == ObstacleType::Tree)
            {
                // Unconditional block: even if jumping, you stop
                BlockAgainst(obstacleCollision);
                continue;
            }

            // ==========================================
            // 2. FENCES & ROCKS: Block, but CAN be jumped over
            // ==========================================
            if (type == ObstacleType::Fence || type == ObstacleType::Rock)
            {
                if (!isJumping) // Only block if NOT jumping
                {
                    BlockAgainst(obstacleCollision);
                }
                continue;
            }

            // ==========================================
            // 3. PALISADE: Blocks, can jump over, BUT jumping over damages you
            // ==========================================
            if (type == ObstacleType::Palisade)
            {
                if (isJumping)
                {
                    // Jumping over the Palisade deals 1 damage!
                    if (!pawn->HasShield() && !pawn->IsImmuneToHazards())
                    {
                        ApplyDamage(1.0f, glm::vec3(0.0f, -1.0f, 0.0f), 0.3f);
                    }
                    else if (pawn->HasShield())
                    {
                        pawn->ConsumeShield();
                    }
                }
                else // Not jumping, just walking into it
                {
                    BlockAgainst(obstacleCollision);
                }
                continue;
            }

            // ==========================================
            // 4. SPIKES: Damage + Knockback (Always hurts)
            // ==========================================
            if (type == ObstacleType::Spikes)
            {
                if (pawn->HasShield())
                {
                    pawn->ConsumeShield();
                }
                else if (!pawn->IsImmuneToHazards())
                {
                    ApplyDamage(1.0f, glm::vec3(0.0f, 0.0f, 1.0f), 0.5f);
                }
                continue;
            }

            // ==========================================
            // 5. MUD: Slow + Lingers 2 seconds
            // ==========================================
            if (type == ObstacleType::Mud)
            {
                if (onSlowApplied)
                {
                    onSlowApplied(3.0f, 0.5f, true);
                }
                continue;
            }

            // ==========================================
            // 6. BUSHES: Slow ONLY while inside (No linger)
            // ==========================================
            if (type == ObstacleType::Bush)
            {
                if (onSlowApplied)
                {
                    onSlowApplied(0.5f, 0.4f, false);
                }
                continue;
            }
        }
    }
}

void HazardCollision::CheckAreaHazards(
    const std::vector<std::unique_ptr<MovingHazard>>& hazards)
{
    if (!pawn)
        return;

    glm::vec3 pawnPos = pawn->GetTransform().GetPosition();

    for (const auto& hazard : hazards)
    {
        if (!hazard || hazard->HasExpired())
            continue;

        // Check for fire patches
        if (hazard->GetMovementPattern() == HazardMovementPattern::TemporaryZone)
        {
            const GroundEntity& visual = hazard->GetVisual();
            const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);

            if (hazardVisual && hazardVisual->GetType() == HazardType::Fireball)
            {
                if (hazard->IsActive())
                {
                    float distance = glm::length(pawnPos - visual.GetGroundPosition());
                    float burnRadius = 0.6f;

                    if (distance < burnRadius)
                    {
                        if (pawn->HasShield())
                        {
                            pawn->ConsumeShield();
                        }
                        else if (!pawn->IsImmuneToHazards())
                        {
                            ApplyDamage(0.2f, glm::vec3(0.0f), 0.0f);
                        }
                    }
                }
            }
        }

        // Check for lightning strikes
        if (hazard->GetMovementPattern() == HazardMovementPattern::WarningThenStrike)
        {
            const GroundEntity& visual = hazard->GetVisual();
            const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);

            if (hazardVisual && hazardVisual->GetType() == HazardType::Lightning && hazard->IsActive())
            {
                float distance = glm::length(pawnPos - visual.GetGroundPosition());
                float strikeRadius = 0.5f;

                if (distance < strikeRadius)
                {
                    if (pawn->HasShield())
                    {
                        pawn->ConsumeShield();
                    }
                    else if (!pawn->IsImmuneToHazards())
                    {
                        ApplyDamage(1.0f, glm::vec3(0.0f, 1.0f, 0.0f), 0.5f);
                    }
                }
            }
        }
    }
}

bool HazardCollision::ApplyDamage(float damage, const glm::vec3& direction, float knockback)
{
    if (damageCooldownRemaining > 0.0f)
        return false;

    if (damage <= 0.0f && knockback <= 0.0f)
        return false;

    // Apply damage
    if (damage > 0.0f && onDamageTaken)
        onDamageTaken(damage, direction);

    // Apply knockback
    if (knockback > 0.0f && onKnockback)
    {
        glm::vec3 knockbackDir = direction;
        if (glm::length(knockbackDir) < 0.01f)
            knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);
        knockbackDir = glm::normalize(knockbackDir);

        // --- THE FIX: Add an upward bounce so the pawn leaves the ground! ---
        glm::vec3 bounceDir = glm::normalize(glm::vec3(knockbackDir.x, 0.8f, knockbackDir.z));
        // --------------------------------------------------------------------

        onKnockback(bounceDir * knockback, false);
    }

    damageCooldownRemaining = damageCooldown;
    return true;
}

void HazardCollision::RefreshPawnVolume()
{
    if (!pawn)
        return;

    // Recalculate the pawn's collision box size every frame
    const float pawnWidth = 0.4f;
    const float pawnHeight = 0.6f;

    pawnCollision.SetBox(
        pawn->GetTransform().GetPosition(),
        glm::vec3(pawnWidth * 0.4f, pawnHeight * 0.4f, pawnWidth * 0.4f));
}

void HazardCollision::BlockAgainst(const CollisionBox& obstacle)
{
    CollisionComponent component;
    component.SetBox(obstacle.center, obstacle.halfExtents);

    BlockAgainst(component);
}

void HazardCollision::BlockAgainstBoxes(const std::vector<CollisionBox>& boxes)
{
    if (!pawn)
        return;

    for (const CollisionBox& box : boxes)
    {
        // Re-read the pawn between volumes. Resolving against one box moves
        // it, so testing the rest against a stale position would push it
        // back out of a wall it is no longer inside - which is how a corner
        // between two solids turns into a rattle.
        RefreshPawnVolume();

        if (!pawnCollision.Intersects(
            CollisionBox{ box.center, box.halfExtents }))
        {
            continue;
        }

        BlockAgainst(box);
    }
}

void HazardCollision::BlockAgainst(const CollisionComponent& obstacle)
{
    if (!pawn)
        return;

    // Exactly the penetration depth, along the axis the pawn is least buried
    // in. Resting flush on the surface means the next frame finds no overlap
    // and pushes nothing, which is what stops the shaking.
    const glm::vec3 push = pawnCollision.ResolveHorizontalOverlap(obstacle);

    if (std::abs(push.x) < 1e-5f && std::abs(push.z) < 1e-5f)
        return;

    const glm::vec3 resolved = pawn->GetTransform().GetPosition() + push;

    // The listener owns the pawn, so it applies the position and decides what
    // to do with the velocity. Handing it the push as well is what lets it
    // cancel only the blocked axis and leave the pawn free to slide.
    if (onBlocked)
        onBlocked(resolved, push);
}
