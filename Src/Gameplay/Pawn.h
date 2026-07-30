#pragma once

#include <glm.hpp>

#include "GroundShadow.h"
#include "WorldObject.h"

/// The player's chess pawn, currently a placeholder cube.
///
/// It deliberately has no movement, collision, health or abilities yet: it
/// exists so the camera has something to frame and so the starting position
/// from GDD section 4 is already correct. Movement will arrive as an
/// Update override, which the base class already calls every frame.
class Pawn : public WorldObject
{
public:

    Pawn();

    /// Builds the placeholder mesh and applies the spawn pose.
    void Initialize() override;

    /// Sets the point on the ground the pawn stands on. The pawn's own
    /// height is added automatically, so callers work in ground coordinates.
    void SetSpawnPosition(const glm::vec3& groundPosition);

    const glm::vec3& GetSpawnPosition() const;

    /// Returns the pawn to its spawn point. Checkpoints will reuse this by
    /// calling SetSpawnPosition first.
    void Respawn();

    /// The pawn owns its shadow and keeps it underneath itself, so it will
    /// follow along for free once the pawn can move.
    const WorldObject& GetShadow() const;

private:

    /// Puts the shadow back under the pawn's current position.
    void UpdateShadow();

    glm::vec3 spawnPosition;

    /// Y of the ground the pawn is standing on, which the shadow sits on.
    float groundHeight;

    GroundShadow shadow;
};
