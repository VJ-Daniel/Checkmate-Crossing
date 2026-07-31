#pragma once

#include "GroundEntity.h"

/// Stationary battlefield props (GDD section 4).
///
/// These shape the path: some block movement, some slow the pawn, some just
/// dress the field. None of that behaviour exists yet - this only names the
/// kinds so their models can be built and placed.
enum class ObstacleType
{
    Tree,
    Rock,
    Fence,
    Wall,
    Bush,
    Spikes,
    Cow,
    Palisade
};

/// Moving-hazard identifiers from GDD section 2.
///
/// Movement lives in MovingHazard, never in this shared taxonomy. Arrow,
/// Spear, Cannonball, RollingRock and RollingLog currently have reusable 3D
/// assets. Fireball and Lightning deliberately remain meshless until their
/// planned sprite/billboard visuals are available.
enum class HazardType
{
    Arrow,
    Spear,
    Cannonball,
    RollingRock,
    RollingLog,
    Fireball,
    Lightning
};

constexpr int ObstacleTypeCount = 8;

constexpr int HazardTypeCount = 7;

ObstacleType ObstacleTypeFromIndex(int index);

HazardType HazardTypeFromIndex(int index);

const char* GetObstacleTypeName(ObstacleType type);

const char* GetHazardTypeName(HazardType type);

/// Base for a renderable obstacle asset instance, stationary or hazard.
///
/// This layer owns visual state only: mesh, material, transform and shadow.
/// Future gameplay objects will reference one of these visuals and own
/// movement, collision, damage, timers, animation and effects separately.
class Obstacle : public GroundEntity
{
public:

    ~Obstacle() override = default;

    /// Human-readable name, for logging and later for a level editor.
    virtual const char* GetName() const = 0;
};

/// A stationary prop: a tree, wall, spike bed and so on.
class StaticObstacle : public Obstacle
{
public:

    explicit StaticObstacle(ObstacleType type);

    ObstacleType GetType() const;

    const char* GetName() const override;

private:

    ObstacleType type;
};

/// The visual anchor referenced by a MovingHazard.
///
/// Airborne models are authored at flight height, while rolling hazards rest
/// on the ground. Sprite-deferred hazard types may have no mesh, but still use
/// this transform-only anchor so movement remains separate from rendering.
class Hazard : public Obstacle
{
public:

    explicit Hazard(HazardType type);

    HazardType GetType() const;

    const char* GetName() const override;

private:

    HazardType type;
};
