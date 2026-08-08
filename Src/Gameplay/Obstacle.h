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

    /// The burning patch a fireball leaves where it landed.
    ///
    /// Its own type rather than "a Fireball that stopped moving": the
    /// projectile and the fire it creates are two different hazards with
    /// different lifetimes, different damage rules and different visuals,
    /// and telling them apart by movement pattern meant every system that
    /// touched either had to re-derive which one it was looking at.
    FloorFire,

    /// The patch of broken ground a thrown spear leaves where it lands.
    ///
    /// Its own type for the same reason FloorFire is: the spear in flight and
    /// the danger it leaves behind have different lifetimes, different damage
    /// rules and different visuals, and telling them apart by movement
    /// pattern makes every system that touches either re-derive which one it
    /// is looking at.
    SpearImpact,

    Lightning,
    Cow
};

constexpr int ObstacleTypeCount = 9;

constexpr int HazardTypeCount = 10;

ObstacleType ObstacleTypeFromIndex(int index);

HazardType HazardTypeFromIndex(int index);

const char* GetObstacleTypeName(ObstacleType type);

const char* GetHazardTypeName(HazardType type);

/// Whether the Bishop's (and therefore the Queen's) clearing ability may
/// remove this prop.
///
/// The rule is "obstacles and livestock, never projectiles". The ability
/// clears the battlefield's furniture - barricades, walls, rocks, trees,
/// bushes, spike beds - and the animals wandering among it. What it must
/// never do is delete something already in flight: a player who banks the
/// Bishop is buying a path through the scenery, not a panic button that
/// erases the arrow heading for them.
///
/// Every ObstacleType qualifies, including the cow, which the GDD lists
/// among the stationary hazards the player has to navigate around even
/// though it chases. The projectile side of the rule is enforced by the
/// HazardType overload below, not here.
bool IsAbilityClearable(ObstacleType type);

/// Whether the ability may remove a hazard of this type.
///
/// This is the half of the rule that protects projectiles, and it is
/// deliberately a whitelist of one: only the cow can be cleared. Arrows,
/// spears, cannonballs, rolling rocks and logs, fireballs, the fire they
/// leave and lightning are all off limits, in flight or not.
bool IsAbilityClearable(HazardType type);

/// Whether this prop is a physical solid that stops things moving into it.
///
/// The battlefield's structure: walls, trees, fences, rocks and palisades.
/// Mud, bushes and spike beds are deliberately not solid - they are ground
/// the player crosses at a cost rather than something to be stopped by, and
/// a boulder should roll straight over all three.
///
/// Distinct from the player's own blocking rules, which additionally decide
/// what can be jumped and what hurts on the way over. This is only "is there
/// something physically in the way".
bool IsSolidObstacle(ObstacleType type);

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
