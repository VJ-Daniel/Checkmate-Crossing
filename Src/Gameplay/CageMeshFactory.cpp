/*
    ============================================================
    Checkmate Crossing - Cage Mesh Factory

    Builds the visual-only frame and hinge-local door leaves for the King's
    placeholder prison. Gameplay and door animation live outside
    this reusable mesh library.
    ============================================================
*/

#include "CageMeshFactory.h"

#include <algorithm>

#include "MeshBuilder.h"
#include "ObstacleMeshFactory.h"

using namespace CageMetrics;

namespace
{
    enum class DoorLeaf
    {
        Left,
        Right
    };

    const glm::vec3 CageTimber = ObstaclePalette::DarkWood * 0.96f;
    const glm::vec3 CageTimberAccent = ObstaclePalette::Wood * 0.90f;
    const glm::vec3 CageIron = ObstaclePalette::Iron * 0.72f;
    const glm::vec3 CageDarkIron = ObstaclePalette::Iron * 0.52f;

    void AddDoorBox(
        MeshBuilder& builder,
        float leftX,
        float rightX,
        float bottomY,
        float topY,
        float backZ,
        float frontZ)
    {
        builder.AddBox(
            glm::vec3(
                (leftX + rightX) * 0.5f,
                (bottomY + topY) * 0.5f,
                (backZ + frontZ) * 0.5f),
            glm::vec3(
                rightX - leftX,
                topY - bottomY,
                frontZ - backZ));
    }

    /// Clips a box from the original full-width door at the centre seam and
    /// converts it to the selected leaf's hinge-local coordinates. With both
    /// leaves closed, the two clipped meshes reproduce the old gate exactly.
    void AddDoorLeafBox(
        MeshBuilder& builder,
        DoorLeaf leaf,
        float fullLeftX,
        float fullRightX,
        float bottomY,
        float topY,
        float backZ,
        float frontZ)
    {
        const float leafMinimum = (leaf == DoorLeaf::Left)
            ? 0.0f
            : DoorLeafWidth;

        const float leafMaximum = (leaf == DoorLeaf::Left)
            ? DoorLeafWidth
            : DoorWidth;

        const float clippedLeft = std::max(fullLeftX, leafMinimum);
        const float clippedRight = std::min(fullRightX, leafMaximum);

        if (clippedRight <= clippedLeft)
            return;

        const float hingeX = (leaf == DoorLeaf::Left)
            ? 0.0f
            : DoorWidth;

        AddDoorBox(
            builder,
            clippedLeft - hingeX,
            clippedRight - hingeX,
            bottomY,
            topY,
            backZ,
            frontZ);
    }
}

namespace CageMeshFactory
{
    CagePartModel Create(CagePart part)
    {
        MeshBuilder builder;
        CagePartModel model;

        switch (part)
        {
        case CagePart::Frame:
        {
            const float halfWidth = Width * 0.5f;
            const float halfDepth = Depth * 0.5f;
            const float postInset = PostWidth * 0.5f;

            // A single stone plinth keeps the cage visibly grounded without
            // changing the flat terrain beneath it.
            builder.SetColor(ObstaclePalette::DarkStone);
            builder.AddSlab(Width, Depth, 0.0f, BaseHeight);

            // The four posts carry both the side bars and the open roof.
            builder.SetColor(CageTimber);
            for (int xSide = -1; xSide <= 1; xSide += 2)
            {
                for (int zSide = -1; zSide <= 1; zSide += 2)
                {
                    builder.AddSlabAt(
                        static_cast<float>(xSide) *
                            (halfWidth - postInset),
                        static_cast<float>(zSide) *
                            (halfDepth - postInset),
                        PostWidth,
                        PostWidth,
                        BaseHeight,
                        RoofBottom);
                }
            }

            builder.SetColor(CageIron);

            // Three broad bars close the back without stacking a dense set
            // of thin lines behind the door and obscuring the King.
            constexpr int BackBarCount = 3;
            for (int bar = 0; bar < BackBarCount; ++bar)
            {
                const float x =
                    -InteriorWidth * 0.5f +
                    InteriorWidth * static_cast<float>(bar + 1) /
                        static_cast<float>(BackBarCount + 1);

                builder.AddSlabAt(
                    x,
                    -halfDepth + BarWidth * 0.5f,
                    BarWidth,
                    BarWidth,
                    BaseHeight,
                    RoofBottom);
            }

            // Two bars on either side complete the enclosure. The front
            // bars belong to the separate closed door mesh.
            constexpr int SideBarCount = 2;
            for (int xSide = -1; xSide <= 1; xSide += 2)
            {
                for (int bar = 0; bar < SideBarCount; ++bar)
                {
                    const float z =
                        -InteriorDepth * 0.5f +
                        InteriorDepth * static_cast<float>(bar + 1) /
                            static_cast<float>(SideBarCount + 1);

                    builder.AddSlabAt(
                        static_cast<float>(xSide) *
                            (halfWidth - BarWidth * 0.5f),
                        z,
                        BarWidth,
                        BarWidth,
                        BaseHeight,
                        RoofBottom);
                }
            }

            // A timber perimeter and three crossbars suggest a roof without
            // turning the cage into a solid hut or hiding the King.
            builder.SetColor(CageTimber);
            builder.AddSlabAt(
                0.0f, -halfDepth + postInset,
                Width, PostWidth,
                RoofBottom, Height);
            builder.AddSlabAt(
                0.0f, halfDepth - postInset,
                Width, PostWidth,
                RoofBottom, Height);
            builder.AddSlabAt(
                -halfWidth + postInset, 0.0f,
                PostWidth, InteriorDepth,
                RoofBottom, Height);
            builder.AddSlabAt(
                halfWidth - postInset, 0.0f,
                PostWidth, InteriorDepth,
                RoofBottom, Height);

            builder.SetColor(CageTimberAccent);
            constexpr float RoofCrossbarWidth = 0.07f;
            for (int crossbar = -1; crossbar <= 1; ++crossbar)
            {
                builder.AddSlabAt(
                    static_cast<float>(crossbar) * Width * 0.25f,
                    0.0f,
                    RoofCrossbarWidth,
                    InteriorDepth,
                    RoofBottom + 0.04f,
                    Height - 0.03f);
            }

            model.height = Height;
            model.width = Width;
            model.depth = Depth;
            break;
        }

        case CagePart::LeftDoor:
        case CagePart::RightDoor:
        {
            constexpr float StileWidth = 0.10f;
            constexpr float RailHeight = 0.11f;

            const DoorLeaf leaf = (part == CagePart::LeftDoor)
                ? DoorLeaf::Left
                : DoorLeaf::Right;

            const auto addDoorBox =
                [&builder, leaf](
                    float leftX,
                    float rightX,
                    float bottomY,
                    float topY,
                    float backZ,
                    float frontZ)
                {
                    AddDoorLeafBox(
                        builder,
                        leaf,
                        leftX,
                        rightX,
                        bottomY,
                        topY,
                        backZ,
                        frontZ);
                };

            // Build from the original full-width coordinates, clipping every
            // box at the centre seam inside addDoorBox. The resulting local
            // origin is the correct outer hinge for each leaf.
            builder.SetColor(CageTimber);
            addDoorBox(
                0.0f, StileWidth,
                0.0f, DoorHeight,
                0.0f, DoorThickness);
            addDoorBox(
                DoorWidth - StileWidth, DoorWidth,
                0.0f, DoorHeight,
                0.0f, DoorThickness);

            builder.SetColor(CageTimberAccent);
            addDoorBox(
                0.0f, DoorWidth,
                0.0f, RailHeight,
                0.0f, DoorThickness);
            addDoorBox(
                0.0f, DoorWidth,
                DoorHeight - RailHeight, DoorHeight,
                0.0f, DoorThickness);

            // Three broad iron bars fill the framed opening.
            builder.SetColor(CageIron);
            constexpr int DoorBarCount = 3;
            const float innerWidth = DoorWidth - StileWidth * 2.0f;
            for (int bar = 0; bar < DoorBarCount; ++bar)
            {
                const float centerX =
                    StileWidth +
                    innerWidth * static_cast<float>(bar + 1) /
                        static_cast<float>(DoorBarCount + 1);

                addDoorBox(
                    centerX - BarWidth * 0.5f,
                    centerX + BarWidth * 0.5f,
                    RailHeight,
                    DoorHeight - RailHeight,
                    0.012f, 0.048f);
            }

            // One central strap ties the bars together. Two shorter straps
            // mark the future hinge side; they are detail, not behavior.
            builder.SetColor(CageDarkIron);
            constexpr float StrapHeight = 0.075f;
            addDoorBox(
                StileWidth, DoorWidth - StileWidth,
                DoorHeight * 0.5f - StrapHeight * 0.5f,
                DoorHeight * 0.5f + StrapHeight * 0.5f,
                0.045f, DoorThickness);

            for (int hinge = 0; hinge < 2; ++hinge)
            {
                const float centerY =
                    (hinge == 0) ? DoorHeight * 0.27f : DoorHeight * 0.73f;

                addDoorBox(
                    0.0f, 0.25f,
                    centerY - StrapHeight * 0.5f,
                    centerY + StrapHeight * 0.5f,
                    0.045f, DoorThickness);
            }

            // A single raised plate at the free edge reads as a medieval
            // latch without implying that interaction exists yet.
            addDoorBox(
                DoorWidth - 0.18f, DoorWidth - 0.07f,
                DoorHeight * 0.5f - 0.08f,
                DoorHeight * 0.5f + 0.08f,
                0.042f, DoorThickness);

            model.height = DoorHeight;
            model.width = DoorLeafWidth;
            model.depth = DoorThickness;
            break;
        }
        }

        model.mesh = builder.Build();

        return model;
    }
}

CageMeshLibrary::CageMeshLibrary()
{
}

const CagePartModel& CageMeshLibrary::GetModel(CagePart part)
{
    CagePartModel& model = models[static_cast<int>(part)];

    if (!model.mesh)
        model = CageMeshFactory::Create(part);

    return model;
}

void CageMeshLibrary::Clear()
{
    for (CagePartModel& model : models)
        model = CagePartModel();
}
