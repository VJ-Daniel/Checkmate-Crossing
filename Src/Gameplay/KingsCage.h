#pragma once

#include <memory>

#include <glm.hpp>

#include "CageMeshFactory.h"
#include "GroundEntity.h"
#include "WorldObject.h"
#include "Collision.h"


/// Visual-only placeholder for the cage holding the king at a level's goal.
///
/// The fixed frame uses this GroundEntity's mesh and transform. The two barred
/// door leaves are separate WorldObjects whose origins are authored on their
/// outer hinges, so rescue gameplay can rotate them without rebuilding the
/// cage. This class does not animate the doors or add collision, interaction,
/// or win-condition behavior.
class KingsCage : public GroundEntity
{
public:

    KingsCage();

    /// Assigns the library's shared frame and two door-leaf meshes.
    ///
    /// Both doors start closed at exactly zero rotation. Building the cage
    /// requires the same live GL context as the project's other mesh assets.
    void Build(CageMeshLibrary& meshes);

    /// Moves the frame and its separate doors as one visual assembly.
    ///
    /// Only positions are updated, so each door rotation remains intact
    /// if the cage itself is repositioned.
    void SetGroundPosition(const glm::vec3& groundPosition) override;

    /// The separately transformable left leaf, or null before Build.
    WorldObject* GetLeftDoor();

    const WorldObject* GetLeftDoor() const;

    /// The separately transformable right leaf, or null before Build.
    WorldObject* GetRightDoor();

    const WorldObject* GetRightDoor() const;

    /// Height of the solid base on which the captive king should stand.
    static float GetFloorHeight();

    CollisionBox GetCollisionBox() const;

private:

    std::shared_ptr<WorldObject> leftDoor;

    std::shared_ptr<WorldObject> rightDoor;
};
