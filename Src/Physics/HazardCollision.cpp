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

            if (type == HazardType::Spear)
            {
                // Thrown on an arc now rather than swept along the lane, so
                // its volume is centred on the shaft where it actually is
                // rather than on the ground beneath it. Kept small: the
                // spear is meant to be dodged in the air and answered for on
                // the ground, and a generous airborne box would make the
                // landing zone pointless.
                const glm::vec3 center =
                    visual.GetGroundPosition() +
                    glm::vec3(0.0f, GameConfig::SpearHitRadius, 0.0f);

                comp.SetCircle(center, GameConfig::SpearHitRadius);
            }
            else if (type == HazardType::Arrow)
            {
                // Long and thin - use a smaller box
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
            else if (type == HazardType::Fireball)
            {
                // Meshless: it is drawn as a sprite, so UpdateFromEntity has
                // no model to measure and would leave a zero-sized box the
                // pawn could walk straight through. The radius comes from
                // config instead, centred on the projectile's flight height
                // rather than the ground beneath it.
                const glm::vec3 center =
                    visual.GetGroundPosition() +
                    glm::vec3(0.0f, GameConfig::FireballHitRadius, 0.0f);

                comp.SetCircle(center, GameConfig::FireballHitRadius);
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
                // The puddle is intentionally almost flat visually, but its
                // gameplay volume stays 0.3 high so crossing it reliably
                // registers the slow while matching the visible footprint.
                const glm::vec3 mudCenter =
                    position + glm::vec3(0.0f, 0.15f, 0.0f);

                comp.SetBox(
                    mudCenter,
                    glm::vec3(0.475f, 0.15f, 0.425f));
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
    CheckAreaHazards(movingHazards, deltaTime);
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

        // Standing zones - the fireball's floor fire and lightning's strike
        // area - belong to CheckAreaHazards, which tests them by radius.
        // Letting them fall through here as well would run two damage paths
        // over one hazard: the second is swallowed by the damage cooldown
        // today, so it costs nothing visible, but it is the kind of thing
        // that starts double-hitting the moment the cooldown is retuned.
        const HazardMovementPattern pattern = hazard->GetMovementPattern();

        if (pattern == HazardMovementPattern::TemporaryZone ||
            pattern == HazardMovementPattern::WarningThenStrike)
        {
            continue;
        }

        const GroundEntity& visual = hazard->GetVisual();

        // Skip if the pawn is immune
        if (pawn->IsImmuneToHazards())
            continue;

        CollisionComponent hazardCollision = GetHazardCollision(*hazard);

        if (pawnCollision.Intersects(hazardCollision))
        {
            const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);
            const StaticObstacle* obstacleVisual =
                dynamic_cast<const StaticObstacle*>(&visual);

            const bool isCow = obstacleVisual &&
                obstacleVisual->GetType() == ObstacleType::Cow;

            const HazardType type = isCow
                ? HazardType::Cow
                : (hazardVisual
                    ? hazardVisual->GetType()
                    : HazardType::Arrow);

            // Determine damage and effects based on hazard type
            float damage = 1.0f;
            float knockback = 1.5f;
            glm::vec3 knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);

            // Get the hazard's velocity for knockback direction
            glm::vec3 velocity = hazard->GetVelocity();
            if (glm::length(velocity) > 0.01f)
                knockbackDir = glm::normalize(velocity);

            // The cow is a moving environmental blocker rather than a
            // damaging projectile. Mark its knockback separately so the
            // pawn's brief post-hit protection behaves as intended.
            if (isCow)
            {
                ApplyDamage(0.0f, knockbackDir, knockback, true);
                continue;
            }

            // If shield is available, consume it instead of taking damage
            if (pawn->HasShield())
            {
                pawn->ConsumeShield();
                // Still apply knockback, but no damage
                ApplyDamage(0.0f, knockbackDir, knockback * 0.5f);
                continue;
            }

            // Per-type damage and knockback, decided before the single
            // ApplyDamage call below.
            //
            // These used to be a second ApplyDamage stacked on top of a
            // generic first one. Only the first ever landed - the second
            // arrived inside its own cooldown and was dropped - so the
            // per-type numbers were being written but never applied.
            if (type == HazardType::Fireball)
            {
                damage = GameConfig::FireballDamage;
                knockback = GameConfig::FireballKnockback;
            }
            else if (type == HazardType::Spear)
            {
                damage = GameConfig::SpearDamage;
                knockback = GameConfig::SpearKnockback;
            }
            else if (type == HazardType::Cannonball)
            {
                damage = 0.5f;
                knockback = 2.5f;
            }
            else if (type == HazardType::RollingRock)
            {
                // Rolling Rock: "large hit area, easier to see, harder to move around"
                damage = 1.0f;
                knockback = 2.0f; // High damage, strong push
            }
            else if (type == HazardType::RollingLog)
            {
                // Rolling Log: Similar damage, slightly lighter
                damage = 0.5f;
                knockback = 2.5f;
            }

            ApplyDamage(damage, knockbackDir, knockback);
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

bool HazardCollision::UpdateFloorFireContact(
    const MovingHazard& hazard,
    const glm::vec3& pawnPosition,
    float deltaTime)
{
    // Measured on the ground plane: the patch burns whoever is standing in
    // it, and a jump does not clear it.
    const glm::vec3 offset =
        pawnPosition - hazard.GetVisual().GetGroundPosition();

    const float distance = glm::length(
        glm::vec3(offset.x, 0.0f, offset.z));

    if (distance >= GameConfig::FloorFireRadius || !hazard.IsActive())
        return false;

    auto entry = fireContactTimers.find(&hazard);

    // Not in the map means this is the frame the pawn stepped in, so the
    // first tick is immediate rather than a second late.
    const bool firstContact = entry == fireContactTimers.end();

    if (firstContact)
    {
        entry = fireContactTimers.emplace(&hazard, 0.0f).first;
    }
    else
    {
        entry->second += deltaTime;
    }

    if (!firstContact && entry->second < GameConfig::FloorFireDamageInterval)
        return true;

    entry->second = 0.0f;

    if (pawn->HasShield())
    {
        pawn->ConsumeShield();
    }
    else if (!pawn->IsImmuneToHazards())
    {
        // No knockback: being pushed out of a fire the player walked into
        // would undo the hazard. The tick rate is this timer's job, not the
        // shared hit cooldown's -- that cooldown is shorter than a second,
        // so leaving the pacing to it burned far faster than once a second.
        ApplyDamage(
            GameConfig::FloorFireDamage,
            glm::vec3(0.0f),
            0.0f);
    }

    return true;
}

void HazardCollision::CheckAreaHazards(
    const std::vector<std::unique_ptr<MovingHazard>>& hazards,
    float deltaTime)
{
    if (!pawn)
        return;

    const glm::vec3 pawnPos = pawn->GetTransform().GetPosition();

    // Which fires the pawn is standing in this frame. Anything with a timer
    // that is not in here has been left, and its timer is dropped below so
    // walking back in starts the burn from the beginning.
    std::vector<const MovingHazard*> touchedFires;

    for (const auto& hazard : hazards)
    {
        if (!hazard || hazard->HasExpired())
            continue;

        const GroundEntity& visual = hazard->GetVisual();
        const Hazard* hazardVisual = dynamic_cast<const Hazard*>(&visual);

        if (!hazardVisual)
            continue;

        // Floor fire: burns once on contact, then once a second while stood
        // in. Its own type now, so this no longer has to ask which movement
        // pattern a Fireball happens to be using.
        if (hazardVisual->GetType() == HazardType::FloorFire)
        {
            if (UpdateFloorFireContact(*hazard, pawnPos, deltaTime))
                touchedFires.push_back(hazard.get());

            continue;
        }

        // Broken ground where a spear struck. One hit while it lasts, paced
        // by the shared cooldown rather than the fire's own timer: this is a
        // spot to be driven off, not an area to be ground down in.
        if (hazardVisual->GetType() == HazardType::SpearImpact)
        {
            if (!hazard->IsActive())
                continue;

            const glm::vec3 offset = pawnPos - visual.GetGroundPosition();

            const float distance = glm::length(
                glm::vec3(offset.x, 0.0f, offset.z));

            if (distance < GameConfig::SpearImpactRadius)
            {
                if (pawn->HasShield())
                {
                    pawn->ConsumeShield();
                }
                else if (!pawn->IsImmuneToHazards())
                {
                    ApplyDamage(
                        GameConfig::SpearImpactDamage,
                        glm::vec3(0.0f),
                        0.0f);
                }
            }

            continue;
        }

        // Lightning: the warning phase is harmless, and whether the strike
        // caught the pawn was settled by MovingHazard at the instant the
        // countdown ended. Consuming the hit here applies it exactly once,
        // however many frames the bolt stays on screen.
        //
        // Every zone is an independent hazard in this list, so any number of
        // them can be warning and striking at once.
        if (hazardVisual->GetType() == HazardType::Lightning)
        {
            if (!hazard->ConsumeStrikeHit())
                continue;

            if (pawn->HasShield())
            {
                pawn->ConsumeShield();
            }
            else if (!pawn->IsImmuneToHazards())
            {
                ApplyDamage(
                    GameConfig::LightningDamage,
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    GameConfig::LightningKnockback);
            }
        }
    }

    // Retire the timers of every fire the pawn is no longer in, including
    // any that expired out from under them.
    for (auto it = fireContactTimers.begin(); it != fireContactTimers.end(); )
    {
        const bool stillTouching =
            std::find(touchedFires.begin(), touchedFires.end(), it->first) !=
            touchedFires.end();

        it = stillTouching ? std::next(it) : fireContactTimers.erase(it);
    }
}

bool HazardCollision::ApplyDamage(
    float damage,
    const glm::vec3& direction,
    float knockback,
    bool isCow)
{
    // --- CRITICAL FIX: If it's the Cow, do NOT set the cooldown! ---
    // This prevents the Cow from making you immune to other hazards.
    if (isCow)
    {
        // Only apply the knockback, and exit immediately!
        if (knockback > 0.0f && onKnockback)
        {
            glm::vec3 knockbackDir = direction;
            if (glm::length(knockbackDir) < 0.01f)
                knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);
            knockbackDir = glm::normalize(knockbackDir);

            glm::vec3 bounceDir = glm::normalize(glm::vec3(knockbackDir.x, 0.8f, knockbackDir.z));
            onKnockback(bounceDir * knockback, true);
        }
        return true; // Exit now, do NOT set damageCooldownRemaining!
    }
    // -------------------------------------------------------------

    // Normal damage logic (Spikes, Arrows, etc.)
    if (damageCooldownRemaining > 0.0f)
        return false;

    if (damage <= 0.0f && knockback <= 0.0f)
        return false;

    if (damage > 0.0f && onDamageTaken)
        onDamageTaken(damage, direction);

    if (knockback > 0.0f && onKnockback)
    {
        glm::vec3 knockbackDir = direction;
        if (glm::length(knockbackDir) < 0.01f)
            knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);
        knockbackDir = glm::normalize(knockbackDir);

        glm::vec3 bounceDir = glm::normalize(glm::vec3(knockbackDir.x, 0.8f, knockbackDir.z));
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
