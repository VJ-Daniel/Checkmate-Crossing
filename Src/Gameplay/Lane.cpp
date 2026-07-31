/*
    ============================================================
    Checkmate Crossing - Lane

    One row of the battlefield. Knows its row index and its purpose in
    the level, which is what obstacles and checkpoints will key off.

    A lane no longer carries a height of its own. The board is one flat
    floor; lane type remains gameplay metadata while physical row chooses
    the subtle mowing stripe. Height is therefore a property of the
    battlefield rather than of the row - which makes "every lane is level
    with every other" true by construction instead of true because ten
    cases happen to agree.
    ============================================================
*/

#include "Lane.h"

#include "GameConfig.h"
#include "Level.h"

Lane::Lane(
    LaneType type,
    int row)
    : type(type),
    row(row)
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
    return GameConfig::GroundSurface;
}
