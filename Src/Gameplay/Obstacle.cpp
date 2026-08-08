/*
    ============================================================
    Checkmate Crossing - Obstacle

    Names the stationary props and moving hazards from the GDD, and
    gives them a common base. Standing on the ground and casting a
    shadow are handled by GroundEntity.

    The models themselves are built by ObstacleMeshFactory.
    ============================================================
*/

#include "Obstacle.h"

ObstacleType ObstacleTypeFromIndex(int index)
{
    switch (index)
    {
    case 0:  return ObstacleType::Tree;
    case 1:  return ObstacleType::Rock;
    case 2:  return ObstacleType::Fence;
    case 3:  return ObstacleType::Wall;
    case 4:  return ObstacleType::Bush;
    case 5:  return ObstacleType::Spikes;
    case 6:  return ObstacleType::Cow;
    case 7:  return ObstacleType::Mud;
    default: return ObstacleType::Palisade;
    }
}

HazardType HazardTypeFromIndex(int index)
{
    switch (index)
    {
    case 0:  return HazardType::Arrow;
    case 1:  return HazardType::Spear;
    case 2:  return HazardType::Cannonball;
    case 3:  return HazardType::RollingRock;
    case 4:  return HazardType::RollingLog;
    case 5:  return HazardType::Fireball;
    case 6:  return HazardType::FloorFire;
    case 7:  return HazardType::SpearImpact;
    case 8:  return HazardType::Lightning;
    default: return HazardType::Cow;
    }
}

const char* GetObstacleTypeName(ObstacleType type)
{
    switch (type)
    {
    case ObstacleType::Tree:     return "Tree";
    case ObstacleType::Rock:     return "Rock";
    case ObstacleType::Fence:    return "Fence";
    case ObstacleType::Wall:     return "Wall";
    case ObstacleType::Bush:     return "Bush";
    case ObstacleType::Spikes:   return "Spikes";
    case ObstacleType::Cow:      return "Cow";
    case ObstacleType::Mud:      return "Mud";
    case ObstacleType::Palisade: return "Palisade";
    }
    return "Unknown";
}

const char* GetHazardTypeName(HazardType type)
{
    switch (type)
    {
    case HazardType::Arrow:       return "Arrow";
    case HazardType::Spear:       return "Spear";
    case HazardType::Cannonball:  return "Cannonball";
    case HazardType::RollingRock: return "Rolling Rock";
    case HazardType::RollingLog:  return "Rolling Log";
    case HazardType::Fireball:    return "Fireball";
    case HazardType::FloorFire:   return "Floor Fire";
    case HazardType::SpearImpact: return "Spear Impact";
    case HazardType::Lightning:   return "Lightning";
    case HazardType::Cow:         return "Cow";
    }
    return "Unknown";
}

bool IsAbilityClearable(ObstacleType type)
{
    switch (type)
    {
        // The battlefield's furniture, plus the livestock wandering in it.
    case ObstacleType::Tree:
    case ObstacleType::Rock:
    case ObstacleType::Fence:
    case ObstacleType::Wall:
    case ObstacleType::Bush:
    case ObstacleType::Spikes:
    case ObstacleType::Mud:
    case ObstacleType::Palisade:
    case ObstacleType::Cow:
        return true;
    }

    // Listed exhaustively above rather than defaulted, so adding a prop to
    // ObstacleType produces a compiler warning here and someone has to
    // decide which side of the rule it falls on.
    return false;
}

bool IsSolidObstacle(ObstacleType type)
{
    switch (type)
    {
    case ObstacleType::Tree:
    case ObstacleType::Rock:
    case ObstacleType::Fence:
    case ObstacleType::Wall:
    case ObstacleType::Palisade:
        return true;

        // Crossed rather than collided with.
    case ObstacleType::Bush:
    case ObstacleType::Mud:
    case ObstacleType::Spikes:
        return false;

        // Not scenery at all - it walks around under its own power.
    case ObstacleType::Cow:
        return false;
    }

    return false;
}

bool IsAbilityClearable(HazardType type)
{
    switch (type)
    {
        // The only hazard that is an animal rather than something thrown.
    case HazardType::Cow:
        return true;

        // Everything in flight, rolling, burning or striking. The ability
        // is not a way to delete an incoming threat.
    case HazardType::Arrow:
    case HazardType::Spear:
    case HazardType::Cannonball:
    case HazardType::RollingRock:
    case HazardType::RollingLog:
    case HazardType::Fireball:
    case HazardType::FloorFire:
    case HazardType::SpearImpact:
    case HazardType::Lightning:
        return false;
    }

    return false;
}

StaticObstacle::StaticObstacle(ObstacleType type)
    : type(type)
{
}

ObstacleType StaticObstacle::GetType() const
{
    return type;
}

void StaticObstacle::SetStructural(bool structuralValue)
{
    structural = structuralValue;
}

bool StaticObstacle::IsStructural() const
{
    return structural;
}

const char* StaticObstacle::GetName() const
{
    return GetObstacleTypeName(type);
}

Hazard::Hazard(HazardType type)
    : type(type)
{
}

HazardType Hazard::GetType() const
{
    return type;
}

const char* Hazard::GetName() const
{
    return GetHazardTypeName(type);
}
