#pragma once

#include "WorldObject.h"

/// The kind of row the pawn is standing on.
///
/// Taken straight from the level layout in GDD section 4. Right now the type
/// only picks the lane's colour, but it is the hook obstacle spawning,
/// checkpoints and the win condition will all read from later.
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
    /// Safe lanes sit raised and hazard lanes sit sunken, so the seam
    /// between them shows a real vertical face. Anything standing on this
    /// lane, the pawn included, rests at this height.
    float GetSurfaceHeight() const;

private:

    LaneType type;

    int row;

    float surfaceHeight;
};
