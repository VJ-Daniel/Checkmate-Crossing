/*
    ============================================================
    Checkmate Crossing - Level

    Builds the battlefield described in GDD section 4: a strip of lanes
    running from the start area, through alternating safe and hazard
    rows and a checkpoint, up to the king's cage.

    Only the ground is created here. The hazards themselves come later.
    ============================================================
*/

#include "Level.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "GameConfig.h"

namespace
{
    enum class BoundaryEdge : std::uint32_t
    {
        Left,
        Right,
        Start,
        End
    };

    /// Six subdued stone/foliage shades. Vertex colour keeps the shared cube
    /// mesh cheap while making the perimeter read as mixed rock and greenery
    /// rather than a repeated single-colour wall.
    const std::array<glm::vec3, 6> BoundaryPalette =
    {
        glm::vec3(0.28f, 0.29f, 0.31f),
        glm::vec3(0.37f, 0.38f, 0.40f),
        glm::vec3(0.46f, 0.47f, 0.48f),
        glm::vec3(0.13f, 0.29f, 0.16f),
        glm::vec3(0.18f, 0.37f, 0.19f),
        glm::vec3(0.25f, 0.43f, 0.23f)
    };

    /// Small integer hash used instead of stateful randomness. A block's
    /// appearance depends only on its edge/layer/index coordinate, so adding
    /// another strip later cannot reshuffle the perimeter that already exists.
    std::uint32_t MixBoundaryHash(std::uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return value;
    }

    float BoundaryValue(
        BoundaryEdge edge,
        int layer,
        int index,
        std::uint32_t channel)
    {
        std::uint32_t key = GameConfig::BoundaryVariationSeed;
        key ^= (static_cast<std::uint32_t>(edge) + 1u) * 0x9e3779b9u;
        key ^= (static_cast<std::uint32_t>(layer) + 1u) * 0x85ebca6bu;
        key ^= (static_cast<std::uint32_t>(index) + 1u) * 0xc2b2ae35u;
        key ^= (channel + 1u) * 0x27d4eb2fu;

        return static_cast<float>(MixBoundaryHash(key) & 0x00ffffffu) /
            static_cast<float>(0x01000000u);
    }

    float BoundaryRange(
        BoundaryEdge edge,
        int layer,
        int index,
        std::uint32_t channel,
        float minimum,
        float maximum)
    {
        return minimum +
            (maximum - minimum) *
            BoundaryValue(edge, layer, index, channel);
    }

    float ClampAxisToFootprint(
        float value,
        float minimum,
        float maximum,
        float halfExtent)
    {
        const float insetMinimum = minimum + std::max(halfExtent, 0.0f);
        const float insetMaximum = maximum - std::max(halfExtent, 0.0f);

        // An oversized object cannot fit on this axis. Centring it is the
        // only stable answer and avoids handing std::clamp inverted bounds.
        if (insetMinimum > insetMaximum)
            return (minimum + maximum) * 0.5f;

        return std::clamp(value, insetMinimum, insetMaximum);
    }

    /// One continuous grass palette for the whole battlefield.
    ///
    /// Colour depends only on the physical row, never on LaneType. This keeps
    /// gameplay sections visually connected while the small value shift
    /// produces regular mowing stripes without textures or extra geometry.
    glm::vec3 GrassColorForRow(int row)
    {
        return (row % 2 == 0)
            ? glm::vec3(0.27f, 0.52f, 0.24f)
            : glm::vec3(0.245f, 0.48f, 0.22f);
    }

}

Level::Level()
    : nextRow(0),
    spawnRow(0)
{
}

void Level::Build()
{
    lanes.clear();
    decorations.clear();
    nextRow = 0;

    // Lanes are solid blocks rather than flat planes, so the battlefield has
    // a real edge all the way round instead of a paper-thin one. One cube is
    // shared by every lane and every decoration, so this still costs a
    // single vertex buffer.
    if (!blockMesh)
        blockMesh = Mesh::CreateCube();

    // ---------------------------------------------------------
    // Level layout, in the order given by the GDD scene diagram.
    // The row counts are the only thing that decides how long each
    // section feels, so tuning the level means editing this block.
    // ---------------------------------------------------------

    // Room behind the pawn so the camera never looks at empty space. The
    // pawn sits low on the screen, so about three rows stay visible behind
    // it and five rows are needed to be safe.
    AddLanes(LaneType::StartArea, 5);

    // The pawn stands on the last start-area row.
    spawnRow = nextRow - 1;

    // The first hazard lane is kept short and close, so the player meets a
    // dangerous row inside the opening view rather than after a long walk
    // across safe ground.
    AddLanes(LaneType::SafeGrass, 1);
    AddLanes(LaneType::Arrow, 2);

    AddLanes(LaneType::SafeGrass, 3);
    AddLanes(LaneType::SpikeMud, 3);

    // GDD: a checkpoint roughly every 20 units of progress.
    AddLanes(LaneType::Checkpoint, 1);

    AddLanes(LaneType::Cannonball, 3);
    AddLanes(LaneType::FenceTree, 3);
    AddLanes(LaneType::FireballLightning, 3);
    AddLanes(LaneType::FinalSafeArea, 3);
    AddLanes(LaneType::KingsCage, 2);

    BuildDecorations();
}

void Level::AddLanes(LaneType type, int count)
{
    for (int i = 0; i < count; ++i)
    {
        const int row = nextRow++;

        auto lane = std::make_shared<Lane>(type, row);

        lane->SetMesh(blockMesh);

        // Every lane block is identical: same height, same thickness, same
        // width. A row differs from its neighbours only in its colour and
        // in which slice of Z it occupies, so the blocks butt together into
        // one unbroken floor with no step, seam or trench anywhere between
        // the start area and the king's cage.
        //
        // The surface still comes from the lane rather than from GameConfig,
        // so this loop keeps working unchanged if a section is ever given a
        // height of its own again.
        const float surface = lane->GetSurfaceHeight();
        const float thickness = GameConfig::BoardBaseThickness;

        // Lanes are drawn far wider than the walkable board so the ground
        // always fills the screen, the way the reference game does.
        lane->GetTransform().SetScale(
            GameConfig::LaneDrawWidth,
            thickness,
            GameConfig::TileSize);

        // The cube is centred on its own origin, so the block is lowered by
        // half its thickness to leave its top exactly at the surface height.
        lane->GetTransform().SetPosition(
            0.0f,
            surface - thickness * 0.5f,
            RowToWorldZ(row));

        // The lane keeps its gameplay type, but its appearance belongs to the
        // continuous field. Alternating physical rows creates subtle,
        // perfectly regular mowing stripes across every level section.
        lane->SetColor(glm::vec4(GrassColorForRow(row), 1.0f));

        lane->Initialize();

        lanes.push_back(lane);
    }
}

void Level::BuildDecorations()
{
    if (!blockMesh || lanes.empty())
        return;

    const LevelBoundsXZ playable = GetPlayableBounds();
    const LevelBoundsXZ visual = GetBoundaryVisualBounds();

    const float layerSpacing =
        GameConfig::BoundaryThickness /
        static_cast<float>(GameConfig::BoundaryLayerCount);

    /// Builds one edge as overlapping rows of independently varied blocks.
    /// No slot is skipped: variation comes from form and colour while the
    /// staggered layers keep the outer world hidden through every corner.
    const auto buildStrip =
        [this, layerSpacing](
            BoundaryEdge edge,
            bool runsAlongX,
            float innerCoordinate,
            float outwardSign,
            float tangentMinimum,
            float tangentMaximum)
        {
            const float span = tangentMaximum - tangentMinimum;
            const int segmentCount = std::max(
                1,
                static_cast<int>(std::ceil(
                    span / GameConfig::BoundaryBlockSpacing)));

            const float segmentSpacing =
                span / static_cast<float>(segmentCount);

            for (int layer = 0;
                layer < GameConfig::BoundaryLayerCount;
                ++layer)
            {
                int previousPaletteIndex = -1;
                const bool isStaggeredLayer = (layer & 1) != 0;
                const int layerSegmentCount =
                    segmentCount + (isStaggeredLayer ? 1 : 0);

                for (int index = 0; index < layerSegmentCount; ++index)
                {
                    float tangent = isStaggeredLayer
                        ? tangentMinimum +
                            static_cast<float>(index) * segmentSpacing
                        : tangentMinimum +
                            (static_cast<float>(index) + 0.5f) * segmentSpacing;

                    // Odd layers explicitly anchor both endpoints; their
                    // interior blocks and every even-layer block keep a small
                    // deterministic jitter. This staggers the seams without
                    // wrapping an endpoint away from a corner.
                    const bool isAnchoredEndpoint =
                        isStaggeredLayer &&
                        (index == 0 || index == layerSegmentCount - 1);

                    if (!isAnchoredEndpoint)
                    {
                        tangent += BoundaryRange(
                            edge,
                            layer,
                            index,
                            0u,
                            -GameConfig::BoundaryAlongJitter,
                            GameConfig::BoundaryAlongJitter);
                    }

                    const float across =
                        innerCoordinate +
                        outwardSign *
                        (static_cast<float>(layer) + 0.5f) * layerSpacing;

                    const float alongSize = BoundaryRange(
                        edge,
                        layer,
                        index,
                        1u,
                        GameConfig::BoundaryMinAlongSize,
                        GameConfig::BoundaryMaxAlongSize);

                    float acrossSize = BoundaryRange(
                        edge,
                        layer,
                        index,
                        2u,
                        GameConfig::BoundaryMinAcrossSize,
                        GameConfig::BoundaryMaxAcrossSize);

                    // Lane meshes end exactly at the start/end Z limits. Make
                    // each cap's innermost row bridge the visual clearance so
                    // no background-colour trench can show between floor and
                    // boundary, while retaining a little overlap with row two.
                    const bool isCap =
                        edge == BoundaryEdge::Start ||
                        edge == BoundaryEdge::End;

                    if (isCap && layer == 0)
                    {
                        acrossSize = std::max(
                            acrossSize,
                            layerSpacing +
                                GameConfig::BoundaryInnerClearance * 2.0f);
                    }

                    const float layerProgress =
                        (GameConfig::BoundaryLayerCount > 1)
                        ? static_cast<float>(layer) /
                            static_cast<float>(
                                GameConfig::BoundaryLayerCount - 1)
                        : 1.0f;

                    // Low rocks/bushes line the playable edge so the pawn
                    // stays readable when pressing into a camera-facing
                    // wall. Height rises irregularly into the outer layers,
                    // where tall blocks hide the world beyond the map.
                    const float layerMaximumHeight =
                        GameConfig::BoundaryMinHeight +
                        (GameConfig::BoundaryMaxHeight -
                            GameConfig::BoundaryMinHeight) *
                        (0.15f + layerProgress * 0.85f);

                    float height = BoundaryRange(
                        edge,
                        layer,
                        index,
                        3u,
                        GameConfig::BoundaryMinHeight,
                        layerMaximumHeight);

                    // The end strip remains solid behind the cage, but its
                    // centre stays low enough to frame rather than hide the
                    // King. Taller stone and greenery remain to either side.
                    if (edge == BoundaryEdge::End && std::fabs(tangent) < 1.4f)
                    {
                        height = std::min(
                            height,
                            0.82f + static_cast<float>(layer) * 0.10f);
                    }

                    int paletteIndex = static_cast<int>(BoundaryValue(
                        edge,
                        layer,
                        index,
                        4u) * static_cast<float>(BoundaryPalette.size()));

                    paletteIndex = std::min(
                        paletteIndex,
                        static_cast<int>(BoundaryPalette.size()) - 1);

                    if (paletteIndex == previousPaletteIndex)
                    {
                        paletteIndex =
                            (paletteIndex + 1 + layer) %
                            static_cast<int>(BoundaryPalette.size());
                    }

                    previousPaletteIndex = paletteIndex;

                    float rotation = BoundaryRange(
                        edge,
                        layer,
                        index,
                        5u,
                        -GameConfig::BoundaryMaxRotationDegrees,
                        GameConfig::BoundaryMaxRotationDegrees);

                    // The enlarged inner cap row meets the floor exactly.
                    // Rotating those blocks would project their long axis a
                    // little onto Z and visibly intrude into the playable
                    // rectangle; the remaining rows retain varied rotation.
                    if (isCap && layer == 0)
                        rotation = 0.0f;

                    auto block = std::make_shared<WorldObject>();
                    block->SetMesh(blockMesh);
                    block->SetColor(glm::vec4(
                        BoundaryPalette[paletteIndex],
                        1.0f));

                    block->GetTransform().SetScale(
                        runsAlongX ? alongSize : acrossSize,
                        height,
                        runsAlongX ? acrossSize : alongSize);

                    block->GetTransform().SetPosition(
                        runsAlongX ? tangent : across,
                        GameConfig::GroundSurface + height * 0.5f,
                        runsAlongX ? across : tangent);

                    block->GetTransform().SetRotation(
                        0.0f,
                        rotation,
                        0.0f);

                    block->Initialize();
                    decorations.push_back(block);
                }
            }
        };

    const float clearance = GameConfig::BoundaryInnerClearance;

    // The caps own the four corner bands. Long sides end at each cap's inner
    // coordinate, avoiding duplicate blocks while preserving overlap there.
    buildStrip(
        BoundaryEdge::Left,
        false,
        playable.minX - clearance,
        -1.0f,
        playable.minZ - clearance,
        playable.maxZ + clearance);

    buildStrip(
        BoundaryEdge::Right,
        false,
        playable.maxX + clearance,
        1.0f,
        playable.minZ - clearance,
        playable.maxZ + clearance);

    buildStrip(
        BoundaryEdge::Start,
        true,
        playable.maxZ + clearance,
        1.0f,
        visual.minX,
        visual.maxX);

    buildStrip(
        BoundaryEdge::End,
        true,
        playable.minZ - clearance,
        -1.0f,
        visual.minX,
        visual.maxX);
}

void Level::Update(float deltaTime)
{
    for (auto& lane : lanes)
    {
        if (lane && lane->IsActive())
            lane->Update(deltaTime);
    }
}

const std::vector<std::shared_ptr<Lane>>& Level::GetLanes() const
{
    return lanes;
}

const std::vector<std::shared_ptr<WorldObject>>& Level::GetDecorations() const
{
    return decorations;
}

int Level::GetSpawnRow() const
{
    return spawnRow;
}

const Lane* Level::GetLane(int row) const
{
    if (row < 0 || row >= static_cast<int>(lanes.size()))
        return nullptr;

    return lanes[row].get();
}

int Level::FindRowOfType(LaneType type) const
{
    for (const auto& lane : lanes)
    {
        if (lane && lane->GetType() == type)
            return lane->GetRow();
    }

    return -1;
}

LevelBoundsXZ Level::GetPlayableBounds() const
{
    LevelBoundsXZ bounds;

    bounds.minX = -GetPlayableHalfWidth();
    bounds.maxX = GetPlayableHalfWidth();

    if (lanes.empty())
        return bounds;

    const float halfTile = GameConfig::TileSize * 0.5f;

    bounds.maxZ = lanes.front()->GetCenterZ() + halfTile;
    bounds.minZ = lanes.back()->GetCenterZ() - halfTile;

    return bounds;
}

LevelBoundsXZ Level::GetBoundaryVisualBounds() const
{
    LevelBoundsXZ bounds = GetPlayableBounds();

    const float expansion =
        GameConfig::BoundaryInnerClearance +
        GameConfig::BoundaryThickness;

    bounds.minX -= expansion;
    bounds.maxX += expansion;
    bounds.minZ -= expansion;
    bounds.maxZ += expansion;

    return bounds;
}

glm::vec3 Level::ClampToPlayableBounds(
    const glm::vec3& position,
    float halfWidth,
    float halfDepth) const
{
    const LevelBoundsXZ bounds = GetPlayableBounds();

    glm::vec3 resolved = position;

    resolved.x = ClampAxisToFootprint(
        resolved.x,
        bounds.minX,
        bounds.maxX,
        halfWidth);

    resolved.z = ClampAxisToFootprint(
        resolved.z,
        bounds.minZ,
        bounds.maxZ,
        halfDepth);

    return resolved;
}

glm::vec3 Level::GetPlayerSpawnPosition() const
{
    // Read off the spawn lane rather than assumed, so the pawn follows the
    // floor wherever it ends up.
    float surface = GameConfig::GroundSurface;

    if (spawnRow >= 0 && spawnRow < static_cast<int>(lanes.size()))
    {
        if (const auto& lane = lanes[spawnRow])
            surface = lane->GetSurfaceHeight();
    }

    return glm::vec3(
        0.0f,
        surface,
        RowToWorldZ(spawnRow));
}

float Level::GetPlayableHalfWidth()
{
    return (GameConfig::BoardWidthInTiles * GameConfig::TileSize) * 0.5f;
}

float Level::RowToWorldZ(int row)
{
    return -static_cast<float>(row) * GameConfig::TileSize;
}
