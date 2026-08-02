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
    case 7:  return ObstacleType::Palisade;
    default: return ObstacleType::Mud;
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
    default: return HazardType::Lightning;
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
    case ObstacleType::Palisade: return "Palisade";
    case ObstacleType::Mud:      return "Mud";
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
    case HazardType::Lightning:   return "Lightning";
    }

    return "Unknown";
}

StaticObstacle::StaticObstacle(ObstacleType type)
    : type(type)
{
}

ObstacleType StaticObstacle::GetType() const
{
    return type;
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
