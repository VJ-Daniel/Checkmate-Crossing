#pragma once

#include <glm.hpp>

#include "WorldObject.h"

/// A dark translucent quad laid flat on the ground beneath a character.
///
/// This stands in for real shadow mapping, which is a much larger change.
/// One draw call per character is enough to anchor it to the ground instead
/// of letting it look like it is floating.
///
/// Anything that stands on the battlefield can own one of these: the player
/// pawn does, and so does every chess piece.
class GroundShadow : public WorldObject
{
public:

    GroundShadow();

    /// Builds the quad. Must be called once a GL context exists.
    void Initialize() override;

    /// Sets the shadow's footprint, normally derived from whatever is
    /// casting it. Depth defaults to width; a fence or a wall needs them
    /// different or its shadow spills across the lane behind it.
    void SetFootprint(float width, float depth = 0.0f);

    /// Places the shadow flat on the given point of the ground, lifted just
    /// clear of the surface so the two do not z-fight.
    void PlaceOn(const glm::vec3& groundPosition);

private:

    float footprintWidth;

    float footprintDepth;
};
