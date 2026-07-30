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

#include <random>

#include "GameConfig.h"

namespace
{
    /// Scenery is kept clear of the walkable board but inside the narrow
    /// band the camera can actually see.
    ///
    /// The yaw makes this band tighter than it looks: moving along +X also
    /// moves a point down the screen, so a block has to satisfy the left or
    /// right screen edge and the top or bottom edge at once. Solving both
    /// leaves roughly |x| between 4.6 and 6.4 - anything further out never
    /// appears on screen at all.
    constexpr float DecorationMinX = 4.8f;
    constexpr float DecorationMaxX = 6.8f;

    /// Every row gets scenery. Only a fraction of it lands inside the view
    /// at any one time, so spacing it out leaves the borders looking bare.
    constexpr int DecorationRowStep = 1;

    /// Blocks per row per side, spread across the band above. Any single
    /// x value only clears both screen edges for a row or two, so the band
    /// has to be filled across its width rather than sampled once.
    constexpr int DecorationsPerSide = 3;

    /// Chance a given slot is left empty, so the border reads as scattered
    /// scenery instead of a solid hedge.
    constexpr float DecorationSkipChance = 0.3f;

    /// Fixed seed: the level looks identical every run, which matters while
    /// tuning the camera.
    constexpr unsigned int DecorationSeed = 20260729u;

    /// Ground colour for each kind of lane.
    ///
    /// Safe ground stays green as the GDD describes, while hazard lanes are
    /// darker and earthier so the player can read the danger before the
    /// obstacles themselves exist.
    glm::vec3 LaneColor(LaneType type)
    {
        switch (type)
        {
        case LaneType::StartArea:
            return glm::vec3(0.42f, 0.60f, 0.30f);

        case LaneType::SafeGrass:
            return glm::vec3(0.24f, 0.48f, 0.22f);

        case LaneType::Arrow:
            return glm::vec3(0.34f, 0.29f, 0.24f);

        case LaneType::SpikeMud:
            return glm::vec3(0.30f, 0.24f, 0.18f);

        case LaneType::Checkpoint:
            return glm::vec3(0.72f, 0.62f, 0.28f);

        case LaneType::Cannonball:
            return glm::vec3(0.34f, 0.34f, 0.36f);

        case LaneType::FenceTree:
            return glm::vec3(0.20f, 0.38f, 0.20f);

        case LaneType::FireballLightning:
            return glm::vec3(0.32f, 0.20f, 0.20f);

        case LaneType::FinalSafeArea:
            return glm::vec3(0.30f, 0.52f, 0.26f);

        case LaneType::KingsCage:
            return glm::vec3(0.42f, 0.36f, 0.55f);
        }

        return glm::vec3(0.5f);
    }

    /// Two families of scenery: dark green woodland and grey battlefield
    /// stone, so the border has some variety without needing textures.
    glm::vec3 DecorationColor(int variant)
    {
        switch (variant)
        {
        case 0:  return glm::vec3(0.16f, 0.34f, 0.17f);
        case 1:  return glm::vec3(0.20f, 0.42f, 0.21f);
        case 2:  return glm::vec3(0.38f, 0.38f, 0.40f);
        default: return glm::vec3(0.30f, 0.30f, 0.32f);
        }
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

    // Lanes are solid blocks, not flat planes, so a raised lane shows a real
    // vertical face where it meets a sunken one. One cube is shared by every
    // lane and every decoration, so this still costs a single vertex buffer.
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

    // The first hazard lane is kept short and close so its far bank lands
    // in the upper middle of the opening view. A sunken lane only shows a
    // vertical face on the side that rises away from the camera, so the
    // bank is the one step the player can actually see at start-up - push
    // it any further out and the whole board reads as flat stripes again.
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

        // The lane block runs from a shared floor up to its own surface
        // height, so the whole level reads as one solid slab of ground with
        // roads cut into it.
        const float surface = lane->GetSurfaceHeight();
        const float thickness = surface + GameConfig::BoardBaseThickness;

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

        // Neighbouring rows alternate slightly in brightness. This is what
        // makes individual lanes readable and gives the chessboard feel the
        // GDD asks for without needing a texture yet.
        glm::vec3 color = LaneColor(type);

        if (row % 2 != 0)
            color *= 0.92f;

        lane->SetColor(glm::vec4(color, 1.0f));

        lane->Initialize();

        lanes.push_back(lane);
    }
}

void Level::BuildDecorations()
{
    std::mt19937 random(DecorationSeed);

    std::uniform_real_distribution<float> heightRange(0.5f, 2.2f);
    std::uniform_real_distribution<float> widthRange(0.55f, 0.95f);
    std::uniform_real_distribution<float> jitterRange(-0.32f, 0.32f);
    std::uniform_real_distribution<float> chanceRange(0.0f, 1.0f);
    std::uniform_int_distribution<int> variantRange(0, 3);

    // Width of one slot in the band, so the blocks spread evenly across it.
    const float slotWidth =
        (DecorationMaxX - DecorationMinX) / DecorationsPerSide;

    for (std::size_t index = 0; index < lanes.size();
        index += DecorationRowStep)
    {
        const auto& lane = lanes[index];

        if (!lane)
            continue;

        // Scenery on both sides of the board, so the border reads from
        // both screen edges no matter how the camera is yawed.
        for (int side = -1; side <= 1; side += 2)
        {
            for (int slot = 0; slot < DecorationsPerSide; ++slot)
            {
                if (chanceRange(random) < DecorationSkipChance)
                    continue;

                auto block = std::make_shared<WorldObject>();

                block->SetMesh(blockMesh);

                const float width = widthRange(random);
                const float height = heightRange(random);

                block->GetTransform().SetScale(width, height, width);

                const float offset =
                    DecorationMinX +
                    slotWidth * (static_cast<float>(slot) + 0.5f) +
                    jitterRange(random);

                block->GetTransform().SetPosition(
                    static_cast<float>(side) * offset,
                    lane->GetSurfaceHeight() + height * 0.5f,
                    lane->GetCenterZ() + jitterRange(random));

                block->SetColor(
                    glm::vec4(DecorationColor(variantRange(random)), 1.0f));

                block->Initialize();

                decorations.push_back(block);
            }
        }
    }
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

glm::vec3 Level::GetPlayerSpawnPosition() const
{
    // The spawn lane is raised, so the pawn has to stand on its surface
    // rather than at y = 0.
    float surface = GameConfig::RaisedLaneSurface;

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
