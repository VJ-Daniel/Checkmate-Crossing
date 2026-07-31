#pragma once

#include <memory>

#include <glm.hpp>

#include "CageMeshFactory.h"
#include "GroundEntity.h"
#include "WorldObject.h"

/// Visual-only placeholder for the cage holding the king at a level's goal.
///
/// The fixed frame uses this GroundEntity's mesh and transform. The barred
/// door is deliberately a separate WorldObject whose origin is authored on
/// its hinge, so future rescue gameplay can rotate it without rebuilding the
/// cage or changing its mesh. This class does not animate the door or add
/// collision, interaction, or win-condition behavior.
class KingsCage : public GroundEntity
{
public:

    KingsCage();

    /// Assigns the library's shared frame and door meshes.
    ///
    /// The door starts closed at exactly zero rotation. Building the cage
    /// requires the same live GL context as the project's other mesh assets.
    void Build(CageMeshLibrary& meshes);

    /// Moves the frame and its separate door as one visual assembly.
    ///
    /// Only positions are updated, so a future door rotation remains intact
    /// if the cage itself is repositioned.
    void SetGroundPosition(const glm::vec3& groundPosition) override;

    /// The separately transformable door, or null before Build is called.
    WorldObject* GetDoor();

    const WorldObject* GetDoor() const;

    /// Height of the solid base on which the captive king should stand.
    static float GetFloorHeight();

private:

    std::shared_ptr<WorldObject> door;
};
