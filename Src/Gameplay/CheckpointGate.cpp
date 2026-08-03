/*
    ============================================================
    Checkmate Crossing - Checkpoint Gate

    Assembles the modular gate from GateMeshFactory's parts and hangs
    its two leaves on their hinges.

    Everything the class does comes down to one decision: a door leaf
    is authored around its own hinge, so hanging it is placing its
    origin on the pillar face and nothing else. Opening it is then a
    rotation about Y with no offset to apply and no correction to keep
    in step - which is exactly what an animation wants to inherit.
    ============================================================
*/

#include "CheckpointGate.h"

#include <algorithm>
#include <cmath>

namespace
{
    /// Shadows sit slightly inside the footprint of a structure, the same
    /// as the props do.
    constexpr float GateShadowScale = 0.95f;

    /// How far a banner stands off the face it hangs on.
    constexpr float BannerStandoff = 0.03f;

    /// Every second wall segment is dropped a shade.
    ///
    /// One mesh repeated down the board is stone laid by a machine. The
    /// object colour multiplies the mesh's own vertex tints, so a segment
    /// can be knocked back without a second mesh or a second material.
    constexpr float WallSegmentShade = 0.95f;
}

GatePiece::GatePiece()
    : localOffset(0.0f)
{
}

void GatePiece::SetLocalOffset(const glm::vec3& offset)
{
    localOffset = offset;
}

const glm::vec3& GatePiece::GetLocalOffset() const
{
    return localOffset;
}

void GatePiece::PlaceRelativeTo(const glm::vec3& gateGroundPosition)
{
    transform.SetPosition(gateGroundPosition + localOffset);
}

CheckpointGate::CheckpointGate()
    : doorAngle(0.0f)
{
}

void CheckpointGate::Build(
    GateMeshLibrary& meshes,
    const CheckpointGateLayout& gateLayout)
{
    layout = gateLayout;

    parts.clear();
    leftDoor.reset();
    rightDoor.reset();

    // ---------------------------------------------------------
    // Pillars, standing hard against the opening.
    // ---------------------------------------------------------

    AddPart(
        meshes.GetModel(GatePart::LeftPillar),
        glm::vec3(-GateMetrics::PillarCenterX, 0.0f, 0.0f));

    AddPart(
        meshes.GetModel(GatePart::RightPillar),
        glm::vec3(GateMetrics::PillarCenterX, 0.0f, 0.0f));

    if (layout.topCaps)
    {
        // The cap is authored from its own base, so topping a pillar is
        // simply standing one at the height the shaft finished at.
        for (int side = -1; side <= 1; side += 2)
        {
            AddPart(
                meshes.GetModel(GatePart::StoneCap),
                glm::vec3(
                    static_cast<float>(side) * GateMetrics::PillarCenterX,
                    GateMetrics::PillarHeight,
                    0.0f));
        }
    }

    // ---------------------------------------------------------
    // The hinges.
    //
    // Each leaf's origin goes on the inner face of its pillar, and half a
    // thickness back, which puts the pivot on the leaf's back face rather
    // than down its middle. Both halves of that matter:
    //
    //   - on the pillar face, so the two leaves - each exactly half the
    //     opening wide - meet on the centre line with no gap and no
    //     overlap when they are shut;
    //
    //   - on the back face, so a leaf turning through a right angle sweeps
    //     forward out of the gateway. Pivoting down the middle instead
    //     would drag the back corner of the hinge stile backwards through
    //     the masonry the leaf is hanging on.
    //
    // Shifting the pivot back also leaves the shut leaves centred in the
    // depth of the pillars, which is where they look like they belong.
    // ---------------------------------------------------------

    const float hingeZ = -GateMetrics::DoorThickness * 0.5f;

    leftDoor = AddPart(
        meshes.GetModel(GatePart::LeftDoor),
        glm::vec3(-GateMetrics::DoorWidth, 0.0f, hingeZ));

    rightDoor = AddPart(
        meshes.GetModel(GatePart::RightDoor),
        glm::vec3(GateMetrics::DoorWidth, 0.0f, hingeZ));

    // ---------------------------------------------------------
    // Banners, on the face the pawn walks up to.
    // ---------------------------------------------------------

    if (layout.banners)
    {
        const float bannerZ =
            GateMetrics::PillarDepth * 0.5f + BannerStandoff;

        for (int side = -1; side <= 1; side += 2)
        {
            AddPart(
                meshes.GetModel(GatePart::Banner),
                glm::vec3(
                    static_cast<float>(side) * GateMetrics::PillarCenterX,
                    GateMetrics::BannerHangY,
                    bannerZ));
        }
    }

    // ---------------------------------------------------------
    // Wall run, starting at the outer face of each pillar.
    // ---------------------------------------------------------

    const int segments = std::max(layout.wallSegmentsPerSide, 0);

    for (int side = -1; side <= 1; side += 2)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            const float x =
                GateMetrics::CoreHalfWidth +
                GateMetrics::WallSpacing *
                (static_cast<float>(segment) + 0.5f);

            AddPart(
                meshes.GetModel(GatePart::WallSegment),
                glm::vec3(static_cast<float>(side) * x, 0.0f, 0.0f),
                (segment % 2 == 0) ? 1.0f : WallSegmentShade);
        }
    }

    // ---------------------------------------------------------
    // Measurements and the shadow underneath.
    //
    // One shadow for the whole structure rather than one per piece. The
    // gate is a continuous run of masonry from end to end, so a single
    // strip is both what it would actually cast and one draw call instead
    // of a dozen. An open leaf reaches past the far edge of it, which is a
    // fair trade for a blob shadow standing in for the real thing.
    // ---------------------------------------------------------

    const float halfWidth =
        GateMetrics::CoreHalfWidth +
        GateMetrics::WallSpacing * static_cast<float>(segments);

    SetDimensions(
        GateMetrics::PillarHeight +
        (layout.topCaps ? GateMetrics::CapHeight : 0.0f),
        halfWidth * 2.0f,
        GateMetrics::PillarDepth);

    SetShadowScale(GateShadowScale);

    // Builds the shadow and puts it under wherever the gate already stands.
    // Every piece was placed against that same point as it was added, so
    // rebuilding an already-positioned gate leaves it exactly where it was.
    Initialize();

    SetDoorAngle(doorAngle);
}

std::shared_ptr<GatePiece> CheckpointGate::AddPart(
    const GatePartModel& model,
    const glm::vec3& offset,
    float shade)
{
    auto piece = std::make_shared<GatePiece>();

    piece->SetMesh(model.mesh);

    // White leaves the mesh's own vertex colours untouched, exactly as the
    // props do; anything less knocks the whole piece back a shade.
    piece->SetColor(glm::vec4(shade, shade, shade, 1.0f));

    piece->SetLocalOffset(offset);
    piece->PlaceRelativeTo(groundPosition);

    piece->Initialize();

    parts.push_back(piece);

    return piece;
}

void CheckpointGate::SetGroundPosition(const glm::vec3& groundPosition)
{
    GroundEntity::SetGroundPosition(groundPosition);

    for (const auto& piece : parts)
    {
        if (piece)
            piece->PlaceRelativeTo(groundPosition);
    }
}

void CheckpointGate::SetDoorAngle(float degrees)
{
    // Clamped rather than trusted. Past either end a leaf stops behaving
    // like a door: a negative angle drives its hinge stile straight back
    // into the pillar, and more than a right angle swings it on across the
    // wall run.
    doorAngle = std::min(
        std::max(degrees, 0.0f),
        GateMetrics::MaxDoorAngle);

    // Two lines, and that is the entire hinge.
    //
    // The pivots are the leaves' own origins, so there is no offset to
    // apply and nothing to keep in step - a future animation only has to
    // move this one number. A positive turn about Y carries +X round
    // towards -Z, so the leaf that reaches right from its hinge takes the
    // angle as given and the one that reaches left takes its negative;
    // both then open away from the pawn.
    if (leftDoor)
        leftDoor->GetTransform().SetRotation(0.0f, doorAngle, 0.0f);

    if (rightDoor)
        rightDoor->GetTransform().SetRotation(0.0f, -doorAngle, 0.0f);
}

float CheckpointGate::GetDoorAngle() const
{
    return doorAngle;
}

float CheckpointGate::GetMaxDoorAngle()
{
    return GateMetrics::MaxDoorAngle;
}

float CheckpointGate::GetOpeningWidth()
{
    return GateMetrics::OpeningWidth;
}

GatePiece* CheckpointGate::GetLeftDoor()
{
    return leftDoor.get();
}

GatePiece* CheckpointGate::GetRightDoor()
{
    return rightDoor.get();
}

const std::vector<std::shared_ptr<GatePiece>>& CheckpointGate::GetParts() const
{
    return parts;
}

void CheckpointGate::AppendCollisionBoxes(std::vector<CollisionBox>& out) const
{
    using namespace GateMetrics;

    const glm::vec3 base = groundPosition;

    const int segments = std::max(layout.wallSegmentsPerSide, 0);

    //---------------------------------------------------------
    // The two flagged sides.
    //
    // Each runs from the edge of the opening out to the end of its wall
    // run, so the two boxes stop exactly where the gateway begins and the
    // player can always walk between them. The pillar is the deepest and
    // tallest thing on that side, so it sets the depth and height - erring
    // a few centimetres proud of the thinner wall segments rather than
    // letting the player clip a pillar corner.
    //---------------------------------------------------------

    const float innerX = DoorWidth;
    const float outerX = CoreHalfWidth + WallSpacing * static_cast<float>(segments);

    const float sideHalfWidth = (outerX - innerX) * 0.5f;
    const float sideCenterX = (outerX + innerX) * 0.5f;

    const float sideHalfHeight = PillarHeight * 0.5f;

    for (int side = -1; side <= 1; side += 2)
    {
        CollisionBox box;

        box.center = base + glm::vec3(
            static_cast<float>(side) * sideCenterX,
            sideHalfHeight,
            0.0f);

        box.halfExtents = glm::vec3(
            sideHalfWidth,
            sideHalfHeight,
            PillarDepth * 0.5f);

        out.push_back(box);
    }

    //---------------------------------------------------------
    // The two leaves.
    //
    // A swinging door is not axis aligned, so each leaf's box is rebuilt
    // from its four rotated corners. That is exact at the two poses that
    // matter - shut, where the pair seals the opening, and fully open,
    // where both lie back along their pillars and leave the middle clear -
    // and conservatively covers the sweep in between, which is correct:
    // a leaf mid-swing really is in the way.
    //---------------------------------------------------------

    const float angle = glm::radians(doorAngle);

    for (int side = -1; side <= 1; side += 2)
    {
        // Left leaf reaches right from its hinge and turns the positive
        // way; the right leaf mirrors both. Matches SetDoorAngle exactly.
        const float reach = (side < 0) ? DoorWidth : -DoorWidth;
        const float turn = (side < 0) ? angle : -angle;

        const glm::vec3 hinge = base + glm::vec3(
            static_cast<float>(side) * DoorWidth,
            0.0f,
            -DoorThickness * 0.5f);

        const float cosTurn = std::cos(turn);
        const float sinTurn = std::sin(turn);

        float minX = 0.0f;
        float maxX = 0.0f;
        float minZ = 0.0f;
        float maxZ = 0.0f;

        bool first = true;

        for (int corner = 0; corner < 4; ++corner)
        {
            const float localX = (corner & 1) ? reach : 0.0f;
            const float localZ = (corner & 2) ? DoorThickness : 0.0f;

            // The same Y rotation Transform3D applies, so the box cannot
            // disagree with where the leaf is actually drawn.
            const float x = localX * cosTurn + localZ * sinTurn;
            const float z = -localX * sinTurn + localZ * cosTurn;

            if (first)
            {
                minX = maxX = x;
                minZ = maxZ = z;
                first = false;
                continue;
            }

            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }

        CollisionBox box;

        box.center = glm::vec3(
            hinge.x + (minX + maxX) * 0.5f,
            base.y + DoorHeight * 0.5f,
            hinge.z + (minZ + maxZ) * 0.5f);

        box.halfExtents = glm::vec3(
            (maxX - minX) * 0.5f,
            DoorHeight * 0.5f,
            (maxZ - minZ) * 0.5f);

        out.push_back(box);
    }
}

const CheckpointGateLayout& CheckpointGate::GetLayout() const
{
    return layout;
}
