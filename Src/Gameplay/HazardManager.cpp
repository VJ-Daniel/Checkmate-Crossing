/*
    ============================================================
    Checkmate Crossing - Hazard Manager
    Gameplay Programmer: Ayub
    Date: 2026

    See HazardManager.h for an overview.
    ============================================================
*/

#include "HazardManager.h"

#include <algorithm>

#include "GameConfig.h"

HazardManager::HazardManager(ObstacleMeshLibrary& meshLibrary)
    : meshLibrary(meshLibrary)
{
}

MovingHazard& HazardManager::SpawnLinearHazard(
    HazardType type,
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float speed,
    bool loop)
{
    auto visual = meshLibrary.CreateHazard(type);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetLinearSweep(startGroundPosition, endGroundPosition, speed, loop);

    if (type == HazardType::RollingRock ||
        type == HazardType::RollingLog)
    {
        const ObstacleModel& model = meshLibrary.GetModel(type);
        const RollAxisMode axisMode =
            (type == HazardType::RollingLog)
            ? RollAxisMode::AboutX
            : RollAxisMode::FromTravel;

        hazard->EnableRolling(model.boundingRadius, axisMode);
    }

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

MovingHazard& HazardManager::SpawnCurvedHazard(
    HazardType type,
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float speed,
    float curveOffset)
{
    auto visual = meshLibrary.CreateHazard(type);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetCurvedSweep(
        startGroundPosition, endGroundPosition, speed, curveOffset);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

MovingHazard& HazardManager::SpawnArcHazard(
    HazardType type,
    const glm::vec3& startGroundPosition,
    const glm::vec3& endGroundPosition,
    float duration,
    float arcHeight)
{
    auto visual = meshLibrary.CreateHazard(type);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetArcProjectile(
        startGroundPosition, endGroundPosition, duration, arcHeight);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

MovingHazard& HazardManager::SpawnWarningHazard(
    const glm::vec3& groundPosition,
    float warningDuration,
    float strikeDuration,
    float catchRadius)
{
    auto visual = meshLibrary.CreateHazard(HazardType::Lightning);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetWarningThenStrike(
        groundPosition, warningDuration, strikeDuration, catchRadius);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

MovingHazard& HazardManager::SpawnTemporaryZone(
    HazardType type,
    const glm::vec3& groundPosition,
    float duration)
{
    auto visual = meshLibrary.CreateHazard(type);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetTemporaryZone(groundPosition, duration);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

MovingHazard& HazardManager::SpawnCow(
    const glm::vec3& startGroundPosition,
    float maxSpeed)
{
    auto visual = meshLibrary.CreateObstacle(ObstacleType::Cow);
    visual->SetGroundPosition(startGroundPosition);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetFollowTarget(maxSpeed);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

int HazardManager::ClearStationaryObstacles(
    std::vector<std::shared_ptr<StaticObstacle>>& obstacles,
    const glm::vec3& origin,
    float radius,
    int maxCount,
    std::vector<glm::vec3>& clearedPositions,
    std::vector<std::shared_ptr<GroundEntity>>& removedVisuals)
{
    if (maxCount <= 0 || radius <= 0.0f)
        return 0;

    // Gather the eligible ones first, as indices paired with their distance,
    // so the "which are clearable" rule and the "which are nearest" rule stay
    // separate steps rather than one tangled predicate.
    struct Candidate
    {
        std::size_t index = 0;
        float distance = 0.0f;
    };

    std::vector<Candidate> candidates;

    for (std::size_t index = 0; index < obstacles.size(); ++index)
    {
        const auto& obstacle = obstacles[index];

        if (!obstacle ||
            obstacle->IsStructural() ||
            !IsAbilityClearable(obstacle->GetType()))
        {
            continue;
        }

        // Measured on the ground plane. Height would only ever push a tall
        // prop out of range for being tall, which is not the intent.
        const glm::vec3 offset =
            obstacle->GetGroundPosition() - origin;

        const float distance = glm::length(
            glm::vec3(offset.x, 0.0f, offset.z));

        if (distance <= radius)
            candidates.push_back({ index, distance });
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& a, const Candidate& b)
        {
            return a.distance < b.distance;
        });

    const int removeCount = std::min(
        maxCount, static_cast<int>(candidates.size()));

    std::vector<std::size_t> toRemove;
    toRemove.reserve(static_cast<std::size_t>(removeCount));

    for (int slot = 0; slot < removeCount; ++slot)
    {
        const std::size_t index = candidates[slot].index;

        clearedPositions.push_back(obstacles[index]->GetGroundPosition());

        // Handed on rather than dropped: the prop is out of the collision
        // and render lists immediately, but the caller keeps it alive long
        // enough to play its death reaction.
        removedVisuals.push_back(obstacles[index]);

        toRemove.push_back(index);
    }

    // Erase highest index first so earlier erasures don't shift the
    // indices still waiting to be removed.
    std::sort(toRemove.rbegin(), toRemove.rend());

    for (std::size_t index : toRemove)
        obstacles.erase(obstacles.begin() + static_cast<std::ptrdiff_t>(index));

    return removeCount;
}

int HazardManager::ClearRemovableHazards(
    const glm::vec3& origin,
    float radius,
    int maxCount,
    std::vector<glm::vec3>& clearedPositions,
    std::vector<std::shared_ptr<GroundEntity>>& removedVisuals)
{
    if (maxCount <= 0 || radius <= 0.0f)
        return 0;

    struct Candidate
    {
        std::size_t index = 0;
        float distance = 0.0f;
    };

    std::vector<Candidate> candidates;

    for (std::size_t index = 0; index < hazards.size(); ++index)
    {
        const auto& hazard = hazards[index];

        if (!hazard || hazard->HasExpired())
            continue;

        const GroundEntity& visual = hazard->GetVisual();

        // The cow is spawned from the stationary-prop mesh, so it arrives as
        // a StaticObstacle; a hazard-typed one is checked too so this keeps
        // working if it is ever rebuilt as a proper Hazard.
        bool clearable = false;

        if (const auto* obstacle = dynamic_cast<const StaticObstacle*>(&visual))
            clearable = IsAbilityClearable(obstacle->GetType());
        else if (const auto* typed = dynamic_cast<const Hazard*>(&visual))
            clearable = IsAbilityClearable(typed->GetType());

        if (!clearable)
            continue;

        const glm::vec3 offset = visual.GetGroundPosition() - origin;

        const float distance = glm::length(
            glm::vec3(offset.x, 0.0f, offset.z));

        if (distance <= radius)
            candidates.push_back({ index, distance });
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& a, const Candidate& b)
        {
            return a.distance < b.distance;
        });

    const int removeCount = std::min(
        maxCount, static_cast<int>(candidates.size()));

    std::vector<std::size_t> toRemove;
    toRemove.reserve(static_cast<std::size_t>(removeCount));

    for (int slot = 0; slot < removeCount; ++slot)
    {
        const std::size_t index = candidates[slot].index;

        clearedPositions.push_back(
            hazards[index]->GetVisual().GetGroundPosition());

        removedVisuals.push_back(hazards[index]->GetVisualShared());

        toRemove.push_back(index);
    }

    std::sort(toRemove.rbegin(), toRemove.rend());

    for (std::size_t index : toRemove)
        hazards.erase(hazards.begin() + static_cast<std::ptrdiff_t>(index));

    return removeCount;
}

void HazardManager::RegisterRepeatingSpawn(
    float interval,
    std::function<void()> spawnFn)
{
    RepeatingSpawn spawn;
    spawn.interval = interval > 0.0f ? interval : 1.0f;
    spawn.spawnFn = std::move(spawnFn);

    // Fires once immediately so the lane isn't empty until the first
    // interval elapses, then repeats every `interval` seconds from here.
    if (spawn.spawnFn)
        spawn.spawnFn();

    repeatingSpawns.push_back(std::move(spawn));
}

void HazardManager::Update(float deltaTime, const glm::vec3& pawnGroundPosition)
{
    for (RepeatingSpawn& spawn : repeatingSpawns)
    {
        spawn.timer += deltaTime;

        if (spawn.timer >= spawn.interval)
        {
            spawn.timer = 0.0f;

            if (spawn.spawnFn)
                spawn.spawnFn();
        }
    }

    for (auto& hazard : hazards)
    {
        if (hazard)
            hazard->Update(deltaTime, pawnGroundPosition);
    }

    // Collect fireball burn-patch positions in a read-only pass first --
    // spawning while iterating over `hazards` would invalidate it.
    // GDD: "[fireballs] leave fire behind that deals continuous damage
    // over time while the player remains inside it."
    std::vector<glm::vec3> burnPositions;

    for (const auto& hazard : hazards)
    {
        if (!hazard || !hazard->HasExpired())
            continue;

        const auto* fireball = dynamic_cast<const Hazard*>(&hazard->GetVisual());

        // A fireball that finished its flight has landed. The projectile
        // dies here and a separate FloorFire hazard takes over at the impact
        // point -- two objects with two lifetimes, rather than one pretending
        // to be both.
        if (fireball && fireball->GetType() == HazardType::Fireball)
            burnPositions.push_back(hazard->GetVisual().GetGroundPosition());
    }

    hazards.erase(
        std::remove_if(
            hazards.begin(),
            hazards.end(),
            [](const std::unique_ptr<MovingHazard>& hazard)
            {
                return !hazard || hazard->HasExpired();
            }),
        hazards.end());

    for (const glm::vec3& position : burnPositions)
    {
        SpawnTemporaryZone(
            HazardType::FloorFire, position, GameConfig::FireballBurnDuration);
    }
}

const std::vector<std::unique_ptr<MovingHazard>>& HazardManager::GetHazards() const
{
    return hazards;
}

void HazardManager::Clear()
{
    hazards.clear();
    repeatingSpawns.clear();
}