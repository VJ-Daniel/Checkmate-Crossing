#pragma once

#include <array>
#include <memory>

#include "Mesh.h"

/// Shared cage measurements. The frame is centred on X/Z and grounded at
/// y = 0; the door is positioned from the hinge values below.
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
    constexpr float DoorHeight = RoofBottom - BaseHeight;
    constexpr float DoorThickness = 0.06f;

    /// Closed-door placement relative to the cage frame.
    ///
    /// The door is authored hinge-local: every vertex lies in
    /// x = [0, DoorWidth] and z = [0, DoorThickness]. Rotating its instance
    /// around local Y therefore swings it from the left edge without a future
    /// animation system having to rebuild or offset the mesh.
    constexpr float DoorHingeX = -DoorWidth * 0.5f;
    constexpr float DoorHingeY = BaseHeight;
    constexpr float DoorHingeZ = Depth * 0.5f - DoorThickness;
}

/// Independently renderable cage parts. The door is deliberately separate
/// from the static frame so future rescue behavior can animate only its
/// transform; this factory remains visual-only.
enum class CagePart
{
    Frame,
    Door
};

constexpr int CagePartCount = 2;

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
