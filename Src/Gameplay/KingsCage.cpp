/*
    ============================================================
    Checkmate Crossing - King's Cage

    Assembles the visual-only cage from the shared frame and door-leaf
    meshes. Both leaves remain separate objects so gameplay can rotate
    each around its authored outer hinge without changing this asset.
    ============================================================
*/

#include "KingsCage.h"

namespace
{
    /// Position of one authored hinge relative to the cage origin.
    glm::vec3 DoorLocalOffset(float hingeX)
    {
        return glm::vec3(
            hingeX,
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

    const auto buildDoor =
        [this, &meshes](CagePart part, float hingeX)
        {
            const CagePartModel& doorModel = meshes.GetModel(part);

            auto door = std::make_shared<WorldObject>();
            door->SetMesh(doorModel.mesh);
            door->SetColor(glm::vec4(1.0f));

            // Closed is exactly zero. The mesh origin is already on the
            // selected outer hinge, so animation only changes Y rotation.
            door->GetTransform().SetRotation(glm::vec3(0.0f));
            door->GetTransform().SetPosition(
                groundPosition + DoorLocalOffset(hingeX));
            door->Initialize();
            return door;
        };

    leftDoor = buildDoor(
        CagePart::LeftDoor,
        CageMetrics::LeftDoorHingeX);

    rightDoor = buildDoor(
        CagePart::RightDoor,
        CageMetrics::RightDoorHingeX);
}

void KingsCage::SetGroundPosition(const glm::vec3& groundPosition)
{
    GroundEntity::SetGroundPosition(groundPosition);

    if (leftDoor)
    {
        leftDoor->GetTransform().SetPosition(
            groundPosition + DoorLocalOffset(CageMetrics::LeftDoorHingeX));
    }

    if (rightDoor)
    {
        rightDoor->GetTransform().SetPosition(
            groundPosition + DoorLocalOffset(CageMetrics::RightDoorHingeX));
    }
}

WorldObject* KingsCage::GetLeftDoor()
{
    return leftDoor.get();
}

const WorldObject* KingsCage::GetLeftDoor() const
{
    return leftDoor.get();
}

WorldObject* KingsCage::GetRightDoor()
{
    return rightDoor.get();
}

const WorldObject* KingsCage::GetRightDoor() const
{
    return rightDoor.get();
}

float KingsCage::GetFloorHeight()
{
    return CageMetrics::BaseHeight;
}
