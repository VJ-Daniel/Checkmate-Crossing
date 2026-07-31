#pragma once

#include "WorldObject.h"

/// The kind of row the pawn is standing on.
///
/// Taken straight from the level layout in GDD section 4. The type is
/// gameplay metadata for obstacle spawning, checkpoints and the win
/// condition; it does not change the continuous grass appearance.
enum class LaneType
{
    StartArea,
    SafeGrass,
    Arrow,
    SpikeMud,
    Checkpoint,
    Cannonball,
    FenceTree,
    FireballLightning,
    FinalSafeArea,
    KingsCage
};

/// One horizontal row of the battlefield, drawn as a flat coloured strip.
///
/// Lanes are the backbone of the level: obstacles will be attached to a lane
/// and move along it, so each one already knows which row it occupies.
class Lane : public WorldObject
{
public:

    Lane(
        LaneType type,
        int row);

    LaneType GetType() const;

    /// Row index, counting up from the start area toward the king's cage.
    int GetRow() const;

    /// World-space Z of the middle of this lane.
    float GetCenterZ() const;

    /// Height of the walkable top surface of this lane.
    ///
    /// The same for every lane: the battlefield is one continuous flat floor
    /// from the start area to the king's cage. Anything standing on this
    /// lane, the pawn included, rests at this height.
    ///
    /// Still asked of the lane rather than read out of GameConfig directly,
    /// so everything that stands on the board keeps going through the row it
    /// is standing on - and nothing would have to change if a section ever
    /// needed a height of its own again.
    float GetSurfaceHeight() const;

private:

    LaneType type;

    int row;
};
