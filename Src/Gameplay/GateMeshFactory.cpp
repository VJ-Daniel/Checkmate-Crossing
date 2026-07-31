/*
    ============================================================
    Checkmate Crossing - Gate Mesh Factory

    Builds the modular checkpoint gate: two stone pillars, two hinged
    timber leaves, a repeatable wall segment, a capstone and a banner.

    Same rules as the chess pieces and the props: large simple boxes,
    flat colours, sharp edges, one draw call per part.
    ============================================================
*/

#include "GateMeshFactory.h"

#include <algorithm>
#include <cmath>

// The gate is nothing but these measurements arranged, and qualifying every
// one of them buries the shapes under the namespace. Pulled in here only,
// never in a header.
using namespace GateMetrics;

namespace
{
    //---------------------------------------------------------
    // Door space
    //
    // A leaf is authored as if its hinge were the origin: "u" runs from the
    // hinge out to the free edge, and z runs from the hinge plane forward
    // through the timber. Nothing in a leaf is ever given a negative z -
    // that is the invariant the whole hinge depends on, because it is what
    // keeps the leaf inside the quarter turn it can sweep without cutting
    // into the pillar behind it.
    //---------------------------------------------------------

    /// Front and back of each layer of the leaf, measured from the hinge
    /// plane. The rails are full thickness and the bars are set back, so the
    /// two cross visibly instead of merging into one flat panel.
    constexpr float LeafBackZ = 0.0f;
    constexpr float LeafFrontZ = DoorThickness;

    constexpr float BackingFrontZ = DoorBackingThickness;

    constexpr float BarBackZ = 0.02f;
    constexpr float BarFrontZ = DoorThickness - 0.025f;

    constexpr float StudBackZ = DoorThickness - 0.035f;
    constexpr float StudFrontZ = DoorThickness + 0.015f;

    constexpr float BandFrontZ = DoorThickness + 0.02f;

    constexpr float SpikeZ = DoorThickness * 0.5f;

    /// Heraldry stands just proud of the cloth and is sunk into it at the
    /// back, so no two faces of the banner ever end up in the same plane.
    constexpr float EmblemZ = 0.022f;
    constexpr float EmblemDepth = 0.020f;

    /// One box of a door leaf, given in door space and mapped onto world X
    /// by the leaf's side.
    void AddDoorBox(
        MeshBuilder& builder,
        float side,
        float centerU,
        float width,
        float bottomY,
        float topY,
        float backZ,
        float frontZ)
    {
        builder.AddBox(
            glm::vec3(
                side * centerU,
                (bottomY + topY) * 0.5f,
                (backZ + frontZ) * 0.5f),
            glm::vec3(
                width,
                topY - bottomY,
                frontZ - backZ));
    }

    /// Distance from the hinge to the middle of one vertical bar.
    ///
    /// The bars are spread across the gap between the two stiles rather than
    /// across the whole leaf, so the frame stays a frame and the spacing
    /// still comes out even.
    float BarCenterU(int bar)
    {
        const float innerStart = DoorStileWidth;
        const float innerEnd = DoorWidth - DoorStileWidth;

        const float gap =
            (innerEnd - innerStart - DoorBarWidth * DoorBarCount) /
            static_cast<float>(DoorBarCount + 1);

        return innerStart +
            gap * static_cast<float>(bar + 1) +
            DoorBarWidth * (static_cast<float>(bar) + 0.5f);
    }

    /// Height of the middle of one horizontal rail.
    float RailCenterY(int rail)
    {
        const float gap =
            (DoorLatticeHeight - DoorRailHeight * DoorRailCount) /
            static_cast<float>(DoorRailCount + 1);

        return gap * static_cast<float>(rail + 1) +
            DoorRailHeight * (static_cast<float>(rail) + 0.5f);
    }
}

namespace GateMeshFactory
{
    float AddCourses(
        MeshBuilder& builder,
        float x,
        float z,
        float width,
        float depth,
        float bottomY,
        float topY,
        int courses,
        float sideStep)
    {
        courses = std::max(courses, 1);

        if (topY <= bottomY)
            return bottomY;

        const glm::vec3 base = builder.GetColor();

        const float step =
            (topY - bottomY) / static_cast<float>(courses);

        // A stepped course is pushed one step sideways and pulled in by one
        // step on every face. The near face therefore lands two steps in
        // while the far face stays flush, which is what makes the stack read
        // as laid blocks rather than a box with stripes painted on it.
        const float inset = std::fabs(sideStep) * 2.0f;

        for (int course = 0; course < courses; ++course)
        {
            const bool stepped = (course % 2 != 0);

            builder.SetColor(GatePalette::Course(base, course));

            builder.AddSlabAt(
                x + (stepped ? sideStep : 0.0f),
                z,
                stepped ? width - inset : width,
                stepped ? depth - inset : depth,
                bottomY + step * static_cast<float>(course),
                bottomY + step * static_cast<float>(course + 1));
        }

        builder.SetColor(base);

        return topY;
    }

    float AddStoneCap(
        MeshBuilder& builder,
        float bottomY)
    {
        // Two steps, each wider than the one below it. The overhang is the
        // whole difference between a capped pillar and a pillar that simply
        // stops, and it is the same trick ObstacleType::Wall uses.
        builder.SetColor(GatePalette::DarkStone);

        builder.AddSlabAt(
            0.0f, 0.0f,
            CapBandWidth, CapBandDepth,
            bottomY, bottomY + CapBandHeight);

        builder.SetColor(GatePalette::Stone);

        builder.AddSlabAt(
            0.0f, 0.0f,
            CapTopWidth, CapTopDepth,
            bottomY + CapBandHeight, bottomY + CapHeight);

        return bottomY + CapHeight;
    }

    void AddDoorLeaf(
        MeshBuilder& builder,
        float side)
    {
        const float bandTop = DoorLatticeHeight + DoorBandHeight;

        const float hingeStileU = DoorStileWidth * 0.5f;
        const float freeStileU = DoorWidth - DoorStileWidth * 0.5f;

        // ---- Backing ------------------------------------------------
        //
        // One plank behind everything else, in the deepest timber on the
        // gate. It is what fills the holes in the lattice, and the contrast
        // between it and the frame in front is what makes the grid read as
        // crossed beams rather than as a hole cut in a board.
        builder.SetColor(GatePalette::DarkWood * 0.72f);

        AddDoorBox(
            builder, side, DoorWidth * 0.5f, DoorWidth,
            0.0f, DoorLatticeHeight, LeafBackZ, BackingFrontZ);

        // ---- Frame -------------------------------------------------
        //
        // Both stiles are the darkest timber on the leaf. That matters at
        // the free edge in particular: the two leaves meet there, and a dark
        // band down the middle of the gateway is the only thing that tells
        // the player at a glance that this is a pair of doors rather than
        // one panel.
        builder.SetColor(GatePalette::DarkWood);

        AddDoorBox(
            builder, side, hingeStileU, DoorStileWidth,
            0.0f, DoorLatticeHeight, LeafBackZ, LeafFrontZ);

        AddDoorBox(
            builder, side, freeStileU, DoorStileWidth,
            0.0f, DoorLatticeHeight, LeafBackZ, LeafFrontZ);

        AddDoorBox(
            builder, side, DoorWidth * 0.5f, DoorWidth,
            0.0f, DoorSillHeight, LeafBackZ, LeafFrontZ);

        // ---- Lattice ------------------------------------------------
        //
        // Bars behind, rails in front, in two shades. Three tones over two
        // depths is what turns a grid of boxes into crossed timber; drawn
        // flat and in one colour it would read as a fence panel.
        builder.SetColor(GatePalette::Wood * 0.88f);

        for (int bar = 0; bar < DoorBarCount; ++bar)
        {
            AddDoorBox(
                builder, side, BarCenterU(bar), DoorBarWidth,
                0.0f, DoorLatticeHeight, BarBackZ, BarFrontZ);
        }

        builder.SetColor(GatePalette::Wood);

        for (int rail = 0; rail < DoorRailCount; ++rail)
        {
            const float center = RailCenterY(rail);

            AddDoorBox(
                builder, side, DoorWidth * 0.5f, DoorWidth,
                center - DoorRailHeight * 0.5f,
                center + DoorRailHeight * 0.5f,
                LeafBackZ, LeafFrontZ);
        }

        // ---- Ironwork -----------------------------------------------
        //
        // Studs only where a rail crosses a stile. Every crossing would be
        // twenty specks on a leaf less than a tile wide, which at this
        // camera distance is noise rather than detail.
        builder.SetColor(GatePalette::Iron);

        const float studU[2] = { hingeStileU, freeStileU };

        for (const float u : studU)
        {
            for (int rail = 0; rail < DoorRailCount; ++rail)
            {
                const float center = RailCenterY(rail);

                AddDoorBox(
                    builder, side, u, DoorStudSize,
                    center - DoorStudSize * 0.5f,
                    center + DoorStudSize * 0.5f,
                    StudBackZ, StudFrontZ);
            }
        }

        // The strap the spikes are set into. Knocked back from the spikes
        // themselves, which are the same iron catching the light: at full
        // brightness the strap reads as a stone lintel instead of metal.
        builder.SetColor(GatePalette::Iron * 0.80f);

        AddDoorBox(
            builder, side, DoorWidth * 0.5f, DoorWidth,
            DoorLatticeHeight, bandTop, LeafBackZ, BandFrontZ);

        builder.SetColor(GatePalette::Iron);

        // ---- Spikes -------------------------------------------------
        const float spikeGap =
            (DoorWidth - DoorSpikeBase * DoorSpikeCount) /
            static_cast<float>(DoorSpikeCount + 1);

        for (int spike = 0; spike < DoorSpikeCount; ++spike)
        {
            const float u =
                spikeGap * static_cast<float>(spike + 1) +
                DoorSpikeBase * (static_cast<float>(spike) + 0.5f);

            builder.AddFrustumAt(
                side * u, SpikeZ,
                DoorSpikeBase, 0.02f,
                bandTop, DoorHeight);
        }
    }

    void AddRookEmblem(
        MeshBuilder& builder,
        float x,
        float bottomY)
    {
        // A rook read as a tower: a plinth, a shaft and a battlemented top.
        // Five plates is enough for the shape to survive being a fifth of a
        // tile tall, which a faithful chess rook would not.
        builder.SetColor(GatePalette::Emblem);

        builder.AddSlabAt(
            x, EmblemZ, 0.150f, EmblemDepth,
            bottomY, bottomY + 0.040f);

        builder.AddSlabAt(
            x, EmblemZ, 0.100f, EmblemDepth,
            bottomY + 0.040f, bottomY + 0.170f);

        builder.AddSlabAt(
            x, EmblemZ, 0.150f, EmblemDepth,
            bottomY + 0.170f, bottomY + 0.205f);

        for (int merlon = -1; merlon <= 1; ++merlon)
        {
            builder.AddSlabAt(
                x + static_cast<float>(merlon) * 0.052f,
                EmblemZ, 0.036f, EmblemDepth,
                bottomY + 0.205f, bottomY + 0.260f);
        }
    }

    GatePartModel Create(GatePart part)
    {
        MeshBuilder builder;

        GatePartModel model;

        switch (part)
        {
        case GatePart::LeftPillar:
        case GatePart::RightPillar:
        {
            // Mirrored rather than cloned. The courses of one pillar step
            // the opposite way to the other, so the pair reads as a matched
            // gateway instead of the same block stamped twice - and, more
            // importantly, both step away from the opening, leaving the
            // inner faces flat for the leaves to close against.
            const float side =
                (part == GatePart::LeftPillar) ? -1.0f : 1.0f;

            // The footing flares outwards only. Flared both ways it would
            // overhang the gateway, and an open leaf would end up standing
            // through the corner of its own pillar.
            builder.SetColor(GatePalette::DarkStone);

            builder.AddSlabAt(
                side * (PillarBaseWidth - PillarWidth) * 0.5f,
                0.0f,
                PillarBaseWidth, PillarBaseDepth,
                0.0f, PillarBaseHeight);

            builder.SetColor(GatePalette::Stone);

            AddCourses(
                builder,
                0.0f, 0.0f,
                PillarWidth, PillarDepth,
                PillarBaseHeight, PillarHeight,
                PillarCourses,
                side * CourseStep);

            model.height = PillarHeight;
            model.width = PillarBaseWidth;
            model.depth = PillarBaseDepth;
            break;
        }

        case GatePart::LeftDoor:
        case GatePart::RightDoor:
        {
            // The left leaf hangs on the left pillar and reaches right
            // across the opening, and the right leaf does the reverse.
            const float side =
                (part == GatePart::LeftDoor) ? 1.0f : -1.0f;

            AddDoorLeaf(builder, side);

            model.height = DoorHeight;
            model.width = DoorWidth;
            model.depth = DoorThickness;
            break;
        }

        case GatePart::WallSegment:
        {
            // Deliberately the same height, depth and overhang as
            // ObstacleType::Wall, so a run of these meets a wall prop
            // without a step showing at the joint. Only the courses are new.
            builder.SetColor(GatePalette::Stone);

            AddCourses(
                builder,
                0.0f, 0.0f,
                WallBodyWidth, WallBodyDepth,
                0.0f, WallBodyHeight,
                WallCourses,
                CourseStep);

            builder.SetColor(GatePalette::DarkStone);

            builder.AddSlabAt(
                0.0f, 0.0f,
                WallCapWidth, WallCapDepth,
                WallBodyHeight, WallHeight);

            model.height = WallHeight;
            model.width = WallCapWidth;
            model.depth = WallCapDepth;
            break;
        }

        case GatePart::StoneCap:
        {
            AddStoneCap(builder, 0.0f);

            model.height = CapHeight;
            model.width = CapTopWidth;
            model.depth = CapTopDepth;
            break;
        }

        case GatePart::Banner:
        {
            // Authored hanging: y = 0 is the bottom of the swallowtail, so
            // the banner is placed by naming the height its tails reach down
            // to rather than by working back from the rod.
            builder.SetColor(GatePalette::Banner);

            // The notch is the whole reason this reads as a banner instead
            // of a green plank, so it is cut before anything else is added.
            for (int tail = -1; tail <= 1; tail += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(tail) * 0.088f, 0.0f,
                    0.125f, BannerThickness,
                    0.0f, 0.17f);
            }

            builder.AddSlabAt(
                0.0f, 0.0f,
                BannerWidth, BannerThickness,
                0.15f, 0.80f);

            // Each layer overlaps the one below it rather than meeting it
            // edge to edge, so the cloth never shows a seam where two boxes
            // happen to land in the same plane.
            builder.SetColor(GatePalette::BannerFold);

            builder.AddSlabAt(
                0.0f, 0.0f,
                BannerWidth + 0.01f, BannerThickness + 0.01f,
                0.78f, 0.86f);

            builder.SetColor(GatePalette::DarkWood);

            builder.AddSlabAt(
                0.0f, 0.0f,
                BannerRodWidth, 0.06f,
                0.84f, 0.90f);

            builder.SetColor(GatePalette::Banner);

            for (int end = -1; end <= 1; end += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(end) * 0.165f, 0.0f,
                    0.06f, 0.075f,
                    0.825f, 0.915f);
            }

            AddRookEmblem(builder, 0.0f, 0.36f);

            model.height = BannerHeight;
            model.width = BannerRodWidth;
            model.depth = 0.075f;
            break;
        }
        }

        model.mesh = builder.Build();

        return model;
    }
}

GateMeshLibrary::GateMeshLibrary()
{
}

const GatePartModel& GateMeshLibrary::GetModel(GatePart part)
{
    GatePartModel& model = models[static_cast<int>(part)];

    if (!model.mesh)
        model = GateMeshFactory::Create(part);

    return model;
}

void GateMeshLibrary::Clear()
{
    for (GatePartModel& model : models)
        model = GatePartModel();
}
