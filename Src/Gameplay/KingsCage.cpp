/*
    ============================================================
    Checkmate Crossing - King's Cage

    Assembles the visual-only cage from the shared frame and door
    meshes. The door remains a separate object so later gameplay can
    rotate it around its authored hinge without changing this asset.
    ============================================================
*/

#include "KingsCage.h"

namespace
{
    /// Position of the door's authored hinge relative to the cage origin.
    glm::vec3 DoorLocalOffset()
    {
        return glm::vec3(
            CageMetrics::DoorHingeX,
            CageMetrics::DoorHingeY,
            CageMetrics::DoorHingeZ);
    }
}

KingsCage::KingsCage()
{
    // The solid base already anchors the cage to the field. A second,
    // aggregate fake shadow underneath it would only darken the same area.
    SetShadowVisible(false);
}

void KingsCage::Build(CageMeshLibrary& meshes)
{
    const CagePartModel& frameModel =
        meshes.GetModel(CagePart::Frame);

    SetMesh(frameModel.mesh);

    // White preserves the mesh factory's vertex-colour palette.
    SetColor(glm::vec4(1.0f));

    SetDimensions(
        CageMetrics::Height,
        CageMetrics::Width,
        CageMetrics::Depth);

    // GroundEntity still owns a shadow object for the common rendering
    // contract, but it remains invisible because the cage has a solid base.
    Initialize();
    SetShadowVisible(false);

    const CagePartModel& doorModel =
        meshes.GetModel(CagePart::Door);

    door = std::make_shared<WorldObject>();
    door->SetMesh(doorModel.mesh);
    door->SetColor(glm::vec4(1.0f));

    // Closed is exactly zero. A future rescue animation only needs to
    // rotate this transform around Y; the mesh origin is already the hinge.
    door->GetTransform().SetRotation(glm::vec3(0.0f));
    door->GetTransform().SetPosition(
        groundPosition + DoorLocalOffset());
    door->Initialize();
}

void KingsCage::SetGroundPosition(const glm::vec3& groundPosition)
{
    GroundEntity::SetGroundPosition(groundPosition);

    if (door)
    {
        // SetPosition deliberately leaves rotation untouched, preserving any
        // future open-door pose while the complete cage is moved.
        door->GetTransform().SetPosition(
            groundPosition + DoorLocalOffset());
    }
}

WorldObject* KingsCage::GetDoor()
{
    return door.get();
}

const WorldObject* KingsCage::GetDoor() const
{
    return door.get();
}

float KingsCage::GetFloorHeight()
{
    return CageMetrics::BaseHeight;
}
