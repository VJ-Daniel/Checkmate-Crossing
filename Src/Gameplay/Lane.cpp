/*
    ============================================================
    Checkmate Crossing - Lane

    One row of the battlefield. Knows its row index and its purpose in
    the level, which is what obstacles and checkpoints will key off.
    ============================================================
*/

#include "Lane.h"

#include "GameConfig.h"
#include "Level.h"

namespace
{
    /// Safe ground is raised into a slab; hazard lanes are cut down to the
    /// base level so they read as roads and trenches running through it.
    float SurfaceHeightFor(LaneType type)
    {
        switch (type)
        {
        case LaneType::StartArea:
        case LaneType::SafeGrass:
        case LaneType::Checkpoint:
        case LaneType::FenceTree:
        case LaneType::FinalSafeArea:
            return GameConfig::RaisedLaneSurface;

        case LaneType::Arrow:
        case LaneType::SpikeMud:
        case LaneType::Cannonball:
        case LaneType::FireballLightning:
            return GameConfig::SunkenLaneSurface;

        case LaneType::KingsCage:
            // The goal sits highest of all, so it is visible from far away.
            return GameConfig::RaisedLaneSurface + 0.2f;
        }

        return GameConfig::SunkenLaneSurface;
    }
}

Lane::Lane(
    LaneType type,
    int row)
    : type(type),
    row(row),
    surfaceHeight(SurfaceHeightFor(type))
{
}

LaneType Lane::GetType() const
{
    return type;
}

int Lane::GetRow() const
{
    return row;
}

float Lane::GetCenterZ() const
{
    return Level::RowToWorldZ(row);
}

float Lane::GetSurfaceHeight() const
{
    return surfaceHeight;
}
