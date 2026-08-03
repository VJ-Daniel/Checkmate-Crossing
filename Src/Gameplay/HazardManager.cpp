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
    float strikeDuration)
{
    auto visual = meshLibrary.CreateHazard(HazardType::Lightning);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetWarningThenStrike(groundPosition, warningDuration, strikeDuration);

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
    float maxSpeed,
    float detectionRange,
    float followDistance)
{
    auto visual = meshLibrary.CreateObstacle(ObstacleType::Cow);
    visual->SetGroundPosition(startGroundPosition);

    auto hazard = std::make_unique<MovingHazard>(visual);
    hazard->SetFollowTarget(maxSpeed, detectionRange, followDistance);

    hazards.push_back(std::move(hazard));

    return *hazards.back();
}

int HazardManager::RemoveNearest(const glm::vec3& position, int count)
{
    if (count <= 0)
        return 0;

    std::vector<std::size_t> order(hazards.size());

    for (std::size_t index = 0; index < order.size(); ++index)
        order[index] = index;

    std::sort(
        order.begin(),
        order.end(),
        [&](std::size_t a, std::size_t b)
        {
            const float distanceA = glm::length(
                hazards[a]->GetVisual().GetGroundPosition() - position);
            const float distanceB = glm::length(
                hazards[b]->GetVisual().GetGroundPosition() - position);

            return distanceA < distanceB;
        });

    const int removeCount = std::min(
        count, static_cast<int>(order.size()));

    // Erase highest index first so earlier erasures don't shift the
    // indices still waiting to be removed.
    std::vector<std::size_t> toRemove(
        order.begin(), order.begin() + removeCount);

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

        if (fireball &&
            fireball->GetType() == HazardType::Fireball &&
            hazard->GetMovementPattern() == HazardMovementPattern::CurvedSweep)
        {
            burnPositions.push_back(hazard->GetVisual().GetGroundPosition());
        }
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
            HazardType::Fireball, position, GameConfig::FireballBurnDuration);
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