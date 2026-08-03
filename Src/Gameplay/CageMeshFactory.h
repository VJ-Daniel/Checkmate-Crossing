#pragma once

#include <array>
#include <memory>

#include "Mesh.h"

/// Shared cage measurements. The frame is centred on X/Z and grounded at
/// y = 0; both door leaves are positioned from the hinge values below.
namespace CageMetrics
{
    // Sized from the rendered King silhouette: about 1.72 x its full width,
    // 2.02 x its depth and 1.50 x its height, including crown and sword.
    constexpr float Width = 1.20f;
    constexpr float Depth = 0.68f;
    constexpr float Height = 2.20f;

    constexpr float BaseHeight = 0.12f;
    constexpr float RoofBottom = 2.06f;
    constexpr float PostWidth = 0.12f;

    constexpr float InteriorWidth = Width - PostWidth * 2.0f;
    constexpr float InteriorDepth = Depth - PostWidth * 2.0f;
    constexpr float BarWidth = 0.05f;

    constexpr float DoorWidth = InteriorWidth;
    constexpr float DoorLeafWidth = DoorWidth * 0.5f;
    constexpr float DoorHeight = RoofBottom - BaseHeight;
    constexpr float DoorThickness = 0.06f;

    /// Closed-door hinge placement relative to the cage frame.
    ///
    /// Each leaf is authored directly around its outer hinge. The left leaf
    /// occupies local x = [0, DoorLeafWidth], while the right occupies
    /// x = [-DoorLeafWidth, 0]. Their closed geometry joins at x = 0 and
    /// exactly reconstructs the original full-width barred gate.
    constexpr float LeftDoorHingeX = -DoorWidth * 0.5f;
    constexpr float RightDoorHingeX = DoorWidth * 0.5f;
    constexpr float DoorHingeY = BaseHeight;
    constexpr float DoorHingeZ = Depth * 0.5f - DoorThickness;
}

/// Independently renderable cage parts. Both leaves are separate from the
/// static frame so each can rotate around its own hinge; this factory remains
/// visual-only.
enum class CagePart
{
    Frame,
    LeftDoor,
    RightDoor
};

constexpr int CagePartCount = 3;

struct CagePartModel
{
    std::shared_ptr<Mesh> mesh;

    float height = 0.0f;
    float width = 0.0f;
    float depth = 0.0f;
};

/// Builds the blocky placeholder cage meshes from the existing procedural
/// primitives and palette. It owns no interaction, collision, or animation.
namespace CageMeshFactory
{
    CagePartModel Create(CagePart part);
}

/// Lazily builds one shared mesh for each reusable cage part.
///
/// Clear this library while the OpenGL context is still alive.
class CageMeshLibrary
{
public:

    CageMeshLibrary();

    const CagePartModel& GetModel(CagePart part);

    void Clear();

private:

    std::array<CagePartModel, CagePartCount> models;
};
