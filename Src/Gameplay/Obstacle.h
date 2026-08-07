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
    Mud,
    Palisade
};

/// Moving-hazard identifiers from GDD section 2.
///
/// Movement lives in MovingHazard, never in this shared taxonomy. Arrow,
/// Spear, Cannonball, RollingRock and RollingLog currently have reusable 3D
/// assets. Fireball and Lightning are meshless by design: they are drawn as
/// animated sprites by Game rather than by the 3D mesh pass.
enum class HazardType
{
    Arrow,
    Spear,
    Cannonball,
    RollingRock,
    RollingLog,
    Fireball,
    Lightning,
    Cow      
};

constexpr int ObstacleTypeCount = 9;

constexpr int HazardTypeCount = 8; // Updated from 7 to 8

ObstacleType ObstacleTypeFromIndex(int index);

HazardType HazardTypeFromIndex(int index);

const char* GetObstacleTypeName(ObstacleType type);

const char* GetHazardTypeName(HazardType type);

/// Whether the Bishop's (and therefore the Queen's) clearing ability may
/// remove this prop.
///
/// The rule is "stationary only". The ability breaks the battlefield's
/// furniture apart - barricades, walls, rocks, trees, bushes, spike beds -
/// and deliberately does nothing to anything in flight or on the move. A
/// player who banks the Bishop is buying a path through the scenery, not a
/// panic button that deletes the arrow already heading for them.
///
/// Cow is the one exception in this enum, and the reason this predicate
/// exists rather than being "return true". It sits in ObstacleType only
/// because that is where its mesh lives; in play it is a moving hazard
/// driven by a MovingHazard with the FollowTarget pattern, so it falls
/// under the same protection as every other moving hazard.
bool IsAbilityClearable(ObstacleType type);

/// Base for a renderable obstacle asset instance, stationary or moving.
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

    /// Marks this instance as level architecture rather than a breakable
    /// prop, exempting it from the Bishop's clearing ability.
    ///
    /// Needed because a couple of obstacles are load-bearing despite being
    /// ordinary types: the checkpoint gate is fenced off by invisible Wall
    /// instances, and clearing those would let the player walk around the
    /// gate instead of opening it. The type alone cannot tell those apart
    /// from a wall that is genuinely meant to be broken through, so the
    /// distinction is per-instance and set where the obstacle is placed.
    void SetStructural(bool structural);

    bool IsStructural() const;

private:

    ObstacleType type;

    bool structural = false;
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
