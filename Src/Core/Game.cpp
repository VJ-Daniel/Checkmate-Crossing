/*
    ============================================================
    Checkmate Crossing - Game Orchestration Module

    Coordinates the camera, world renderer, battlefield and pawn, and
    owns the order in which they are created, updated and drawn.

    Based on the Gangster Survival OpenGL framework by Leonardo Moura.
    ============================================================
*/

#include "Game.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "GameConfig.h"
#include "ResourceManager.h"
#include "SceneNode.h"
#include "Shader.h"
#include <iostream>  // For debug output

namespace
{
    /// Consecutive lanes of one type, in row order. Generalizes the
    /// "walk forward while the next lane still matches" pattern used by
    /// Game::BuildKingsCage, so each BuildLevel* method below can grab a
    /// whole section's rows in one call instead of repeating that loop.
    std::vector<const Lane*> FindConsecutiveRowsOfType(
        const Level& level,
        LaneType type)
    {
        std::vector<const Lane*> rows;

        const int first = level.FindRowOfType(type);

        if (first < 0)
            return rows;

        int row = first;

        while (const Lane* lane = level.GetLane(row))
        {
            if (lane->GetType() != type)
                break;

            rows.push_back(lane);
            ++row;
        }

        return rows;
    }

    /// Every run of consecutive lanes of one type, in level order.
    ///
    /// FindConsecutiveRowsOfType above stops at the first run, which was
    /// enough while each hazard type appeared exactly once. The level now
    /// revisits types in later sections, and anything still resolving a
    /// single first row would quietly leave those later sections empty -
    /// a whole arrow field with no arrows in it, and no error to say so.
    std::vector<std::vector<const Lane*>> FindRunsOfType(
        const Level& level,
        LaneType type)
    {
        std::vector<std::vector<const Lane*>> runs;

        std::vector<const Lane*> current;

        for (int row = 0; const Lane* lane = level.GetLane(row); ++row)
        {
            if (lane->GetType() == type)
            {
                current.push_back(lane);
                continue;
            }

            if (!current.empty())
            {
                runs.push_back(current);
                current.clear();
            }
        }

        if (!current.empty())
            runs.push_back(current);

        return runs;
    }

    /// Which way a piece is turned when it is only being displayed.
    ///
    /// Presentation, not orientation: every model faces +Z, and this decides
    /// nothing about which way is forward. It exists because a horse seen
    /// head-on is a shapeless lump, so the horse-bodied pieces are stood at
    /// a quarter turn to show their profile while they sit still.
    ///
    /// Applied once when a piece is placed. Anything that later gives a
    /// piece a real heading simply overwrites it.
    float PieceDisplayHeadingDegrees(PieceType type)
    {
        const bool horseBodied =
            type == PieceType::Knight ||
            type == PieceType::MountedPawn;

        return horseBodied ? 90.0f : 0.0f;
    }

    /// Moves a camera axis only when the desired focus leaves its lock
    /// region, and then only enough to put it back on that region's edge.
    float FollowOutsideDeadZone(
        float current,
        float desired,
        float deadZoneSize)
    {
        const float halfZone = std::max(deadZoneSize, 0.0f) * 0.5f;

        if (desired < current - halfZone)
            return desired + halfZone;

        if (desired > current + halfZone)
            return desired - halfZone;

        return current;
    }

    /// Shifts one camera-target axis until its visible footprint fits inside
    /// the corresponding visual bounds. When an extreme viewport is wider
    /// than the available envelope, centring is the only unbiased fallback.
    float FitViewAxis(
        float target,
        float viewMinimum,
        float viewMaximum,
        float allowedMinimum,
        float allowedMaximum)
    {
        const float viewSpan = viewMaximum - viewMinimum;
        const float allowedSpan = allowedMaximum - allowedMinimum;

        if (viewSpan >= allowedSpan)
        {
            const float viewCenter = (viewMinimum + viewMaximum) * 0.5f;
            const float allowedCenter =
                (allowedMinimum + allowedMaximum) * 0.5f;

            return target + allowedCenter - viewCenter;
        }

        if (viewMinimum < allowedMinimum)
            return target + allowedMinimum - viewMinimum;

        if (viewMaximum > allowedMaximum)
            return target - (viewMaximum - allowedMaximum);

        return target;
    }

    constexpr float HazardSpacing = 1.6f;

    std::string NumberedTextureName(
        const std::string& prefix,
        int frame)
    {
        return prefix + std::to_string(frame);
    }

    std::string NumberedTexturePath(
        const std::string& folder,
        const std::string& prefix,
        int frame)
    {
        return "Src/Assets/Sprites/" + folder + "/" +
            prefix + std::to_string(frame) + ".png";
    }
}

Game::Game()
{
    ResourceManager::Initialize();
}

Game::~Game()
{
    Shutdown();
}

bool Game::Initialize(
    float screenWidth,
    float screenHeight)
{
    cameraFollowInitialized = false;

    camera = std::make_shared<Camera3D>(
        screenWidth,
        screenHeight);

    ConfigureCamera();

    auto shader = std::make_shared<Shader>();

    if (!shader->Load(
        "Src/Shaders/world_vertex.glsl",
        "Src/Shaders/world_fragment.glsl"))
    {
        return false;
    }

    renderer = std::make_shared<MeshRenderer>(
        shader,
        camera);

    renderer->Initialize();

    // The sprite pass shares the camera but has its own quad and shader. It is
    // initialized here so a broken sprite shader is caught at start-up rather
    // than the first time something tries to draw a sprite.
    spriteRenderer = std::make_shared<SpriteRenderer>(camera);

    if (!spriteRenderer->Initialize())
        return false;

    spriteRenderer->SetScreenSize(screenWidth, screenHeight);

    // 1. Create the libraries FIRST before anything else uses them.
    pieceMeshes = std::make_shared<PieceMeshLibrary>();

    obstacleMeshes = std::make_shared<ObstacleMeshLibrary>();
    gateMeshes = std::make_shared<GateMeshLibrary>();
    cageMeshes = std::make_shared<CageMeshLibrary>();

    LoadLightningSprites();
    LoadFireballSprites();

    // 2. Build the level
    level = std::make_shared<Level>();
    level->Build();

    // 3. Create the pawn
    pawn = std::make_shared<Pawn>();
    pawn->SetSpawnPosition(level->GetPlayerSpawnPosition());
    pawn->Initialize();

    // 4. Give the pawn the level (for terrain height)
    pawn->SetLevel(level.get());

    // 5. NOW give the pawn the mesh library (which is valid because we created it in Step 1)
    pawn->SetMeshLibrary(pieceMeshes);

    // The rig shares the library's meshes, so it can only be attached once
    // both the library and the pawn exist. The merge had reordered these:
    // Kaung moved library creation to the top of Initialize, which left this
    // call sitting above the line that creates the pawn.
    pawn->AttachRig(*pieceMeshes);

    // ---- Initialize Hazard Collision ----
    hazardCollision = std::make_unique<HazardCollision>(*pawn);

    // 1. Damage callback
    hazardCollision->onDamageTaken = [this](float damage, const glm::vec3& direction) {
        // ULTIMATE SAFETY CHECK: Block damage if the pawn was knocked back by the Cow
        if (pawn->IsKnockedBackByCow())
        {
            return; // The Cow pushed you. You are IMMUNE to damage!
        }

        pawn->TakeDamage(damage);

        // Apply knockback (keep your existing code)
        glm::vec3 currentVel = pawn->GetVelocity();
        glm::vec3 knockback = direction * GameConfig::KnockbackDistance;
        if (glm::length(knockback) > 0.01f) {
            pawn->SetVelocity(currentVel + knockback);
        }
        };

    // 2. Knockback callback (Takes TWO arguments now!)
    hazardCollision->onKnockback = [this](const glm::vec3& knockback, bool isCow) {
        // Forward the knockback to the pawn, passing along the "isCow" flag
        pawn->SetKnockback(knockback, isCow);
        };

    hazardCollision->onBlocked =
        [this](const glm::vec3& position, const glm::vec3& push) {
        pawn->GetTransform().SetPosition(position);

        // Cancel only the axis that was actually blocked. Zeroing the whole
        // velocity stopped the pawn dead against a wall it was merely walking
        // past at an angle; this keeps the tangent component so it slides
        // along the surface, the way it already does against the level bounds.
        glm::vec3 velocity = pawn->GetVelocity();

        if (std::abs(push.x) > 1e-5f)
            velocity.x = 0.0f;

        if (std::abs(push.z) > 1e-5f)
            velocity.z = 0.0f;

        pawn->SetVelocity(velocity);
        };

    hazardCollision->onSlowApplied = [this](float duration, float amount, bool linger) {
        if (linger == false && duration == 0.0f) {
            duration = 0.5f; // Minimum time so the timer registers!
        }
        // --------------------------

        pawn->ApplySlow(duration, amount);
        };

    // Place real stationary level content before the checkpoint gate adds
    // its own solid wall volumes to the same collision-aware collection.
    BuildLevelObstacles();

    BuildCheckpointGate();
    BuildKingsCage();

    hazardManager = std::make_shared<HazardManager>(*obstacleMeshes);
    collectibleManager = std::make_shared<CollectibleManager>(*pieceMeshes);

    BuildLevelHazards();
    BuildLevelCollectibles();

    // Frame the pawn before the first frame is drawn, so it is already in
    // view the moment the window opens.
    UpdateCamera();

    std::cout << "Game initialized with " << stationaryHazards.size() << " stationary hazards." << std::endl;


    return true;
}

void Game::ConfigureCamera()
{
    if (!camera)
        return;

    // The reference game's viewing angle: an orthographic camera tilted
    // down over the lanes, never rotated by the player. Every value comes
    // from GameConfig, where the reasoning behind it is documented.
    camera->SetProjectionMode(ProjectionMode::Orthographic);

    camera->SetPitch(GameConfig::CameraPitchDegrees);
    camera->SetYaw(GameConfig::CameraYawDegrees);
    camera->SetDistance(GameConfig::CameraDistance);

    camera->SetOrthographicHeight(GameConfig::CameraOrthographicHeight);
    camera->SetFieldOfView(GameConfig::CameraFieldOfView);

    camera->SetClipPlanes(
        GameConfig::CameraNearPlane,
        GameConfig::CameraFarPlane);
}

void Game::BuildCheckpointGate()
{
    if (!level || !gateMeshes)
        return;

    checkpointGates.clear();
    checkpointGateOpening.clear();

    for (int row : level->FindRowsOfType(LaneType::Checkpoint))
    {
        const Lane* lane = level->GetLane(row);

        if (!lane)
            continue;

        auto gate = std::make_shared<CheckpointGate>();

        gate->Build(*gateMeshes);

        // Centred on the board and standing on the checkpoint lane's own
        // surface.
        gate->SetGroundPosition(
            glm::vec3(
                0.0f,
                lane->GetSurfaceHeight(),
                lane->GetCenterZ()));

        // =========================================================
        // BLOCK THE ENTIRE GREY WALL (LEFT AND RIGHT OF THE DOOR)
        // =========================================================
        const float gateZ = lane->GetCenterZ();
        const float gateY = lane->GetSurfaceHeight();
        const float doorWidth = 1.2f; // The width of the open door

        // Helper to create an invisible wall using your AddObstacle logic
        auto AddInvisibleWall = [&](float xPos, float width) {
            // Create the obstacle directly
            auto wall = obstacleMeshes->CreateObstacle(ObstacleType::Wall);

            // Make it invisible and as tall as the gate
            wall->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
            wall->SetDimensions(3.5f, width, 0.8f); // 3.5 tall, custom width

            // Place it perfectly over the stone wall
            wall->SetGroundPosition(glm::vec3(xPos, gateY, gateZ + 0.3f)); // Slightly forward to overlap
            wall->SetShadowVisible(false);

            // Part of the gate, not scenery: the Bishop's clearing ability
            // must not be able to delete the barrier and let the player walk
            // around the checkpoint instead of opening it. Applies to every
            // gate now that the level has more than one.
            wall->SetStructural(true);

            wall->Initialize();

            stationaryHazards.push_back(wall);
            };

        // LEFT WALL: Covers everything to the left of the door
        // We place it at the exact midpoint between the door edge and the left map edge
        float leftWallCenter = -doorWidth * 0.5f - 5.0f;
        AddInvisibleWall(leftWallCenter, 10.0f);

        // RIGHT WALL: Covers everything to the right of the door
        // We place it at the exact midpoint between the door edge and the right map edge
        float rightWallCenter = doorWidth * 0.5f + 5.0f;
        AddInvisibleWall(rightWallCenter, 10.0f);

        // =========================================================
        // NOTE: The center gap (-0.6f to 0.6f) is left EMPTY for the door.
        // You will walk right through the door, but the stone walls will stop you!
        // =========================================================

        checkpointGates.push_back(gate);
        checkpointGateOpening.push_back(false);
    }
}
void Game::BuildKingsCage()
{
    if (!level || !pieceMeshes || !cageMeshes)
        return;

    kingsCage.reset();
    capturedKing.reset();

    const int firstRow = level->FindRowOfType(LaneType::KingsCage);

    const Lane* firstLane = level->GetLane(firstRow);

    if (!firstLane)
        return;

    // Centre the objective over the whole consecutive King's Cage area,
    // rather than pinning it to the first row. This is visual placement only:
    // the lane types, counts and gameplay layout remain untouched.
    int lastRow = firstRow;

    while (const Lane* nextLane = level->GetLane(lastRow + 1))
    {
        if (nextLane->GetType() != LaneType::KingsCage)
            break;

        ++lastRow;
    }

    const Lane* lastLane = level->GetLane(lastRow);

    const float centerZ = lastLane
        ? (firstLane->GetCenterZ() + lastLane->GetCenterZ()) * 0.5f
        : firstLane->GetCenterZ();

    const glm::vec3 cageGround(
        0.0f,
        firstLane->GetSurfaceHeight(),
        centerZ);

    kingsCage = std::make_shared<KingsCage>();
    kingsCage->Build(*cageMeshes);
    kingsCage->SetGroundPosition(cageGround);

    // The prisoner stays a normal reusable King model. His ground is the
    // top of the cage's solid base, so his feet do not clip into the plinth.
    capturedKing = pieceMeshes->CreatePiece(
        PieceType::King,
        PieceTeam::White);

    capturedKing->SetGroundPosition(
        cageGround +
        glm::vec3(0.0f, KingsCage::GetFloorHeight(), 0.0f));
}

void Game::BuildLevelObstacles()
{
    if (!level || !obstacleMeshes)
        return;

    stationaryHazards.clear();

    const float halfWidth = Level::GetPlayableHalfWidth();

    // Nothing may be placed at or past the final checkpoint. The run up to
    // the cage is meant to be empty, and enforcing that here rather than by
    // being careful section-by-section means a later layout change cannot
    // accidentally drop a wall across it.
    const std::vector<int> checkpointRows =
        level->FindRowsOfType(LaneType::Checkpoint);

    const int finalCheckpointRow =
        checkpointRows.empty() ? -1 : checkpointRows.back();

    auto place = [this, finalCheckpointRow](
        ObstacleType type,
        float x,
        const Lane& lane)
        {
            if (finalCheckpointRow >= 0 && lane.GetRow() >= finalCheckpointRow)
                return;

            auto obstacle = obstacleMeshes->CreateObstacle(type);

            obstacle->SetGroundPosition(
                glm::vec3(x, lane.GetSurfaceHeight(), lane.GetCenterZ()));

            stationaryHazards.push_back(obstacle);
        };

    // Fills a row edge to edge with one prop type, leaving a single opening.
    //
    // This is what turns the props from scenery into structure: a lone fence
    // in an empty row is walked around without a thought, while a barrier
    // spanning the row forces the player to find the gap. Staggering those
    // gaps between consecutive rows is what makes a section a route rather
    // than a series of straight lines.
    auto placeBarrier = [&place, halfWidth](
        const Lane& lane,
        ObstacleType type,
        float gapCenterX,
        float gapHalfWidth,
        float spacing)
        {
            for (float x = -halfWidth + spacing * 0.5f;
                x <= halfWidth - spacing * 0.5f;
                x += spacing)
            {
                if (std::fabs(x - gapCenterX) < gapHalfWidth)
                    continue;

                place(type, x, lane);
            }
        };

    // Same idea, but alternating two prop types across the span so a barrier
    // reads as something assembled out of whatever was to hand.
    auto placeMixedBarrier = [&place, halfWidth](
        const Lane& lane,
        ObstacleType first,
        ObstacleType second,
        float gapCenterX,
        float gapHalfWidth,
        float spacing)
        {
            int index = 0;

            for (float x = -halfWidth + spacing * 0.5f;
                x <= halfWidth - spacing * 0.5f;
                x += spacing, ++index)
            {
                if (std::fabs(x - gapCenterX) < gapHalfWidth)
                    continue;

                place((index % 2 == 0) ? first : second, x, lane);
            }
        };

    // Wide enough for the pawn (0.4 across) to walk through without
    // scraping, tight enough that it has to be aimed for.
    constexpr float GapHalfWidth = 0.95f;

    //-----------------------------------------------------------
    // Spike and mud fields.
    //-----------------------------------------------------------
    {
        const auto runs = FindRunsOfType(*level, LaneType::SpikeMud);

        // First field: spikes and mud alternate left/right down the section,
        // per the GDD's own "[S] [M]     [S]     [M] [S]" map, so no two
        // consecutive rows block the same corridor.
        if (runs.size() >= 1)
        {
            const auto& rows = runs[0];

            if (rows.size() >= 1)
            {
                place(ObstacleType::Spikes, -halfWidth * 0.55f, *rows[0]);
                place(ObstacleType::Mud, halfWidth * 0.24f, *rows[0]);
                place(ObstacleType::Bush, halfWidth * 0.78f, *rows[0]);
            }

            if (rows.size() >= 2)
            {
                place(ObstacleType::Mud, -halfWidth * 0.24f, *rows[1]);
                place(ObstacleType::Spikes, halfWidth * 0.55f, *rows[1]);
                place(ObstacleType::Rock, -halfWidth * 0.80f, *rows[1]);
            }

            if (rows.size() >= 3)
            {
                place(ObstacleType::Spikes, -halfWidth * 0.70f, *rows[2]);
                place(ObstacleType::Mud, -halfWidth * 0.35f, *rows[2]);
                place(ObstacleType::Spikes, halfWidth * 0.30f, *rows[2]);
                // A gap remains on the far right.
            }
        }

        // Second field, in the harder section: spikes crowd both flanks and
        // the only clean line is a mud crossing, so the shortcut costs speed.
        if (runs.size() >= 2)
        {
            const auto& rows = runs[1];

            if (rows.size() >= 1)
            {
                place(ObstacleType::Spikes, -halfWidth * 0.75f, *rows[0]);
                place(ObstacleType::Spikes, -halfWidth * 0.30f, *rows[0]);
                place(ObstacleType::Mud, halfWidth * 0.25f, *rows[0]);
                place(ObstacleType::Spikes, halfWidth * 0.72f, *rows[0]);
            }

            if (rows.size() >= 2)
            {
                place(ObstacleType::Mud, -halfWidth * 0.60f, *rows[1]);
                place(ObstacleType::Spikes, 0.0f, *rows[1]);
                place(ObstacleType::Mud, halfWidth * 0.60f, *rows[1]);
            }
        }
    }

    //-----------------------------------------------------------
    // Fenced woodland. Both runs are barriers with staggered openings.
    //-----------------------------------------------------------
    {
        const auto runs = FindRunsOfType(*level, LaneType::FenceTree);

        // First woodland: the cow's ground. Openings swing right, left,
        // right, so crossing it means committing to a weave with a cow in
        // the middle of it.
        if (runs.size() >= 1)
        {
            const auto& rows = runs[0];

            if (rows.size() >= 1)
            {
                placeBarrier(
                    *rows[0], ObstacleType::Fence,
                    halfWidth * 0.58f, GapHalfWidth, 1.05f);
            }

            if (rows.size() >= 2)
            {
                // Trees and rocks together: neither can be broken, so this
                // row is purely about finding the line through it.
                placeMixedBarrier(
                    *rows[1], ObstacleType::Tree, ObstacleType::Rock,
                    -halfWidth * 0.42f, GapHalfWidth, 1.15f);
            }

            if (rows.size() >= 3)
            {
                // Walls neither break nor jump (GDD), so this is the row
                // that genuinely cannot be forced - the gap is the only way.
                placeBarrier(
                    *rows[2], ObstacleType::Wall,
                    halfWidth * 0.15f, GapHalfWidth, 1.10f);
            }

            if (rows.size() >= 4)
            {
                placeMixedBarrier(
                    *rows[3], ObstacleType::Palisade, ObstacleType::Bush,
                    -halfWidth * 0.66f, GapHalfWidth, 1.05f);
            }
        }

        // Second woodland: same idea, tighter, and mostly breakable - this
        // is the stretch where a banked Bishop actually buys a shortcut.
        if (runs.size() >= 2)
        {
            const auto& rows = runs[1];

            if (rows.size() >= 1)
            {
                placeBarrier(
                    *rows[0], ObstacleType::Palisade,
                    halfWidth * 0.70f, GapHalfWidth, 1.00f);
            }

            if (rows.size() >= 2)
            {
                placeMixedBarrier(
                    *rows[1], ObstacleType::Fence, ObstacleType::Tree,
                    -halfWidth * 0.22f, GapHalfWidth, 1.10f);
            }

            if (rows.size() >= 3)
            {
                placeMixedBarrier(
                    *rows[2], ObstacleType::Wall, ObstacleType::Fence,
                    halfWidth * 0.40f, GapHalfWidth, 1.10f);
            }
        }
    }

    //-----------------------------------------------------------
    // Scenery on the connecting grass, so the walk between hazard rows is
    // not a bare corridor. Placed well off the centre line and never in a
    // formation, so none of it blocks a route.
    //-----------------------------------------------------------
    {
        const auto runs = FindRunsOfType(*level, LaneType::SafeGrass);

        int runIndex = 0;

        for (const auto& rows : runs)
        {
            // The opening stretch stays clear: the player is still learning
            // to move, and the first arrow row should be what draws the eye.
            if (runIndex >= 2)
            {
                for (std::size_t i = 0; i < rows.size(); ++i)
                {
                    const bool leaning = ((runIndex + i) % 2) == 0;
                    const float side = leaning ? -1.0f : 1.0f;

                    place(
                        (runIndex % 3 == 0)
                        ? ObstacleType::Bush
                        : ObstacleType::Rock,
                        side * halfWidth * 0.86f,
                        *rows[i]);
                }
            }

            ++runIndex;
        }
    }
}

void Game::BuildLevelHazards()
{
    if (!level || !hazardManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // Arrow section (2 rows): row 0 teaches straight dodging, row 1
    // introduces the curved Spear -- there is no dedicated Spear lane
    // type, so it shares the section with Arrow rather than sitting in an
    // unrelated SafeGrass row.
    //
    // Both arrow fields are populated: the second one is a later run of the
    // same lane type, and resolving only the first would leave it bare.
    for (const auto& rows : FindRunsOfType(*level, LaneType::Arrow))
    {
        if (rows.size() >= 1 && rows[0])
        {
            const float y = rows[0]->GetSurfaceHeight();
            const float z = rows[0]->GetCenterZ();

            // "Arrows... range covers the full horizontal width of the
            // map." Loops on its own, so it needs no repeating spawner.
            hazardManager->SpawnLinearHazard(
                HazardType::Arrow,
                glm::vec3(-halfWidth - 0.5f, y, z),
                glm::vec3(halfWidth + 0.5f, y, z),
                3.0f,
                true);
        }

        // A third row, where the field has one, gets a second arrow running
        // the other way so the section cannot be crossed on one rhythm.
        if (rows.size() >= 3 && rows[2])
        {
            const float y = rows[2]->GetSurfaceHeight();
            const float z = rows[2]->GetCenterZ();

            hazardManager->SpawnLinearHazard(
                HazardType::Arrow,
                glm::vec3(halfWidth + 0.5f, y, z),
                glm::vec3(-halfWidth - 0.5f, y, z),
                3.4f,
                true);
        }

        if (rows.size() >= 2 && rows[1])
        {
            const float y = rows[1]->GetSurfaceHeight();
            const float z = rows[1]->GetCenterZ();

            // "Similar speed to arrows, but curved trajectory." One-shot
            // per flight, so a repeating spawner keeps a new one coming.
            hazardManager->RegisterRepeatingSpawn(
                3.5f,
                [this, y, z, halfWidth]()
                {
                    // Straight down its own lane, not bowed.
                    //
                    // The curved sweep offsets the path perpendicular to
                    // itself, and a lane runs along X - so the bow was in Z
                    // and carried the spear a full 1.2 units out of the row
                    // it was fired along. The fireball hit the same problem
                    // and now arcs vertically instead (RegisterFireballVolley),
                    // which is the curve the GDD is describing.
                    hazardManager->SpawnLinearHazard(
                        HazardType::Spear,
                        glm::vec3(-halfWidth - 0.5f, y, z),
                        glm::vec3(halfWidth + 0.5f, y, z),
                        3.2f,
                        false);
                });
        }
    }

    // SpikeMud section: stationary only (BuildLevelObstacles). No moving
    // hazard here, matching the GDD's map.

    // Cannonball section (3 rows): a repeating cannonball sweep at the
    // middle row.
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::Cannonball);

        // A log rolls the length of this section, along Z rather than across
        // it. It used to run through the fireball/lightning finale, which is
        // now reserved for those two hazards alone.
        if (!rows.empty() && rows.front() && rows.back())
        {
            const float y = rows.front()->GetSurfaceHeight();
            const float startZ = rows.front()->GetCenterZ();
            const float endZ = rows.back()->GetCenterZ();
            const float x = halfWidth * 0.44f;

            hazardManager->RegisterRepeatingSpawn(
                3.0f,
                [this, y, x, startZ, endZ]()
                {
                    hazardManager->SpawnLinearHazard(
                        HazardType::RollingLog,
                        glm::vec3(x, y, startZ),
                        glm::vec3(x, y, endZ),
                        2.2f,
                        false);
                });
        }

        if (rows.size() >= 2 && rows[1])
        {
            const float y = rows[1]->GetSurfaceHeight();
            const float z = rows[1]->GetCenterZ();
            const float range = halfWidth * 0.7f;

            // "Faster than arrows... range is shorter and reaches about
            // 70% of the map's width." One-shot per launch.
            hazardManager->RegisterRepeatingSpawn(
                2.5f,
                [this, y, z, range]()
                {
                    hazardManager->SpawnLinearHazard(
                        HazardType::Cannonball,
                        glm::vec3(-range, y, z),
                        glm::vec3(range, y, z),
                        5.5f,
                        false);
                });
        }
    }

    // FenceTree section: the Cow starts chasing from the first row's
    // centre gap, matching the GDD's "moving environmental hazard that
    // follows the player," escalating alongside the stationary props
    // BuildLevelObstacles places here.
    //
    // Playtest feedback: at the original 2.5 it closed the gap too fast to
    // react to. Halved to 1.25.
    //
    // One per woodland, and each is leashed to the checkpoint that closes
    // its section. A cow will otherwise tail the player straight out of its
    // own ground and into whatever comes next, which for the last of them
    // would be the fireball-and-lightning finale.
    {
        const std::vector<int> checkpointRows =
            level->FindRowsOfType(LaneType::Checkpoint);

        const auto runs = FindRunsOfType(*level, LaneType::FenceTree);

        for (const auto& rows : runs)
        {
            if (rows.empty() || !rows.front())
                continue;

            MovingHazard& cow = hazardManager->SpawnCow(
                glm::vec3(
                    0.0f,
                    rows.front()->GetSurfaceHeight(),
                    rows.front()->GetCenterZ()),
                1.25f);

            // The first checkpoint past this woodland is the leash. Rows run
            // toward -Z, so the limit is that lane's Z.
            const int lastRow = rows.back()->GetRow();

            for (int checkpointRow : checkpointRows)
            {
                if (checkpointRow <= lastRow)
                    continue;

                if (const Lane* stop = level->GetLane(checkpointRow))
                    cow.SetFollowLimitZ(stop->GetCenterZ());

                break;
            }
        }
    }

    // FireballLightning section (3 rows): fireballs lobbed in along the
    // outer rows (burn patches are already automatic in
    // HazardManager::Update), Lightning warning zones on the middle row,
    // and RollingLog rolling through the whole span, faster and with a
    // smaller hit area than the Rolling Rock gauntlet below, per the GDD.
    //
    // This is the only place either hazard is set up. Both used to be
    // registered here AND again in two separate blocks further down, which
    // put three fireball volleys and three lightning zones on the same
    // three rows.
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::FireballLightning);

        // Even rows get the fireballs, entering from alternating edges so the
        // section is threatened from both sides rather than one, and odd rows
        // get the lightning. Driven off the row count rather than hardcoded
        // indices, so lengthening the finale fills it instead of leaving the
        // extra rows empty.
        for (std::size_t i = 0; i < rows.size(); i += 2)
        {
            if (!rows[i])
                continue;

            RegisterFireballVolley(
                rows[i]->GetSurfaceHeight(),
                rows[i]->GetCenterZ(),
                (i % 4) == 0);
        }

        for (std::size_t i = 1; i < rows.size(); i += 2)
        {
            if (!rows[i])
                continue;

            const float y = rows[i]->GetSurfaceHeight();
            const float z = rows[i]->GetCenterZ();

            // "An unavoidable strike if the player stays in a marked
            // danger area for too long." The warning phase is the reaction
            // window, and the gaps between zones are the way through.
            //
            // Repeating spawns rather than three immortal zones: a strike
            // now resolves and finishes, so each activation marks its area
            // afresh. Three independent zones on one row, each running its
            // own countdown, so simultaneous warnings and strikes need
            // nothing special from the collision pass.
            const float lightningX[3] =
            {
                -halfWidth * 0.55f,
                0.0f,
                halfWidth * 0.55f
            };

            for (float x : lightningX)
            {
                hazardManager->RegisterRepeatingSpawn(
                    GameConfig::LightningSpawnInterval,
                    [this, x, y, z]()
                    {
                        hazardManager->SpawnWarningHazard(
                            glm::vec3(x, y, z),
                            GameConfig::LightningWarningDuration,
                            GameConfig::LightningStrikeDuration,
                            GameConfig::LightningStrikeRadius);
                    });
            }
        }

        // Nothing else belongs in this section. A rolling log used to run
        // its whole length; it now rolls through the cannonball section
        // instead, so the finale reads as exactly two hazards.
    }

    // Rolling Rock gauntlet: three boulders roll back down the middle of the
    // level, reading as the battlefield collapsing in behind the player
    // rather than a short preview confined to one section.
    //
    // Bounded to the stretch between the first checkpoint and the last
    // woodland. It used to start at the King's Cage and roll the entire
    // field, which sent boulders straight through the finale and across the
    // final approach - both of which are meant to be free of them.
    {
        const auto woodlandRuns = FindRunsOfType(*level, LaneType::FenceTree);
        const int checkpointRow = level->FindRowOfType(LaneType::Checkpoint);

        const Lane* startLane =
            (woodlandRuns.empty() || woodlandRuns.back().empty())
            ? nullptr
            : woodlandRuns.back().back();

        const Lane* endLane = level->GetLane(checkpointRow);

        if (startLane && endLane)
        {
            const float y = startLane->GetSurfaceHeight();
            const float startZ = startLane->GetCenterZ();
            const float endZ = endLane->GetCenterZ();

            const float lanesX[3] =
            {
                -halfWidth * 0.5f,
                0.0f,
                halfWidth * 0.5f
            };

            for (float x : lanesX)
            {
                hazardManager->RegisterRepeatingSpawn(
                    6.0f,
                    [this, y, x, startZ, endZ]()
                    {
                        hazardManager->SpawnLinearHazard(
                            HazardType::RollingRock,
                            glm::vec3(x, y, startZ),
                            glm::vec3(x, y, endZ),
                            1.2f,
                            false);
                    });
            }
        }
    }

    // Fireball and Lightning are set up once, in the FireballLightning
    // block above. Two further blocks used to repeat both here.

}

void Game::RegisterFireballVolley(
    float surfaceHeight,
    float laneZ,
    bool fromLeft)
{
    if (!hazardManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // Launched from just outside the playable width so it is already in
    // flight when it enters the frame, rather than appearing from nothing.
    const float edgeX = halfWidth + GameConfig::FireballSpawnMargin;

    const float startX = fromLeft ? -edgeX : edgeX;

    const float landingX = fromLeft
        ? startX + GameConfig::FireballTravelDistance
        : startX - GameConfig::FireballTravelDistance;

    // Both ends share laneZ, so the whole trajectory stays on the row: the
    // parabola is purely vertical and the fireball cannot drift into a
    // neighbouring lane on its way across.
    const glm::vec3 start(startX, surfaceHeight, laneZ);
    const glm::vec3 landing(landingX, surfaceHeight, laneZ);

    const float duration = GameConfig::FireballTravelDuration;

    hazardManager->RegisterRepeatingSpawn(
        GameConfig::FireballSpawnInterval,
        [this, start, landing, duration]()
        {
            // ArcProjectile, not CurvedSweep: the sweep bows sideways in the
            // ground plane, which pushed the fireball a full curve-offset out
            // of its own row. The arc lifts it in Y instead - the parabola
            // the GDD describes - and leaves X/Z on the straight line between
            // the two lane-aligned endpoints.
            //
            // Landing is where HazardManager drops the floor fire, which is
            // why the arc ends on the ground rather than at flight height.
            hazardManager->SpawnArcHazard(
                HazardType::Fireball,
                start,
                landing,
                duration,
                GameConfig::FireballArcHeight);
        });
}

void Game::BuildLevelCollectibles()
{
    if (!level || !collectibleManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // Bishop (clears nearby moving hazards): the single SafeGrass row
    // right before the Arrow section.
    {
        const int row = level->FindRowOfType(LaneType::Arrow) - 1;

        if (const Lane* lane = level->GetLane(row))
        {
            collectibleManager->Spawn(
                PieceType::Bishop,
                glm::vec3(0.0f, lane->GetSurfaceHeight(), lane->GetCenterZ()));
        }
    }

    // Knight (speed + immunity): the middle row of the SafeGrass x3 block
    // right before SpikeMud.
    {
        const int row = level->FindRowOfType(LaneType::SpikeMud) - 2;

        if (const Lane* lane = level->GetLane(row))
        {
            collectibleManager->Spawn(
                PieceType::Knight,
                glm::vec3(0.0f, lane->GetSurfaceHeight(), lane->GetCenterZ()));
        }
    }

    // Rook (shield): on the Checkpoint row itself, right before Cannonball.
    // Keep it centred on the approach side of the wall: VJ's solid gate
    // frame leaves only the doorway reachable, so Liyyu's original side
    // offset would place the collectible inside the right-hand wall.
    {
        const int row = level->FindRowOfType(LaneType::Checkpoint);

        if (const Lane* lane = level->GetLane(row))
        {
            collectibleManager->Spawn(
                PieceType::Rook,
                glm::vec3(
                    0.0f,
                    lane->GetSurfaceHeight(),
                    lane->GetCenterZ() + GameConfig::TileSize * 0.35f));
        }
    }

    // Queen (combined abilities): the clear gap on the right side of the
    // last FenceTree row, right before the FireballLightning gauntlet --
    // there is no SafeGrass row between those two sections.
    {
        const int row = level->FindRowOfType(LaneType::FireballLightning) - 1;

        if (const Lane* lane = level->GetLane(row))
        {
            collectibleManager->Spawn(
                PieceType::Queen,
                glm::vec3(
                    halfWidth * 0.7f,
                    lane->GetSurfaceHeight(),
                    lane->GetCenterZ()));
        }
    }

    // Same presentation pass as before: the allies stand still until they
    // are picked up, so the horse-bodied ones are shown in profile.
    for (const auto& piece : collectibleManager->GetCollectibles())
    {
        if (piece)
        {
            piece->SetHeadingDegrees(
                PieceDisplayHeadingDegrees(piece->GetType()));
        }
    }
}

void Game::LoadLightningSprites()
{
    for (int frame = 1; frame <= 5; ++frame)
    {
        ResourceManager::LoadTexture(
            NumberedTextureName("lightning_beginning", frame),
            NumberedTexturePath("Lightning", "Lightning_beginning", frame));
    }

    for (int frame = 1; frame <= 3; ++frame)
    {
        ResourceManager::LoadTexture(
            NumberedTextureName("lightning_end", frame),
            NumberedTexturePath("Lightning", "Lightning_end", frame));
    }

    for (int frame = 1; frame <= 10; ++frame)
    {
        ResourceManager::LoadTexture(
            NumberedTextureName("lightning_explosion", frame),
            NumberedTexturePath(
                "Explosion_blue_oval",
                "Explosion_blue_oval",
                frame));
    }

    ResourceManager::LoadTexture(
        "lightning_warning_circle",
        "Src/Assets/Sprites/Warning/red_circle.png");
}

void Game::LoadFireballSprites()
{
    for (int frame = 1; frame <= 8; ++frame)
    {
        const std::string frameNumber =
            frame < 10
            ? "0" + std::to_string(frame)
            : std::to_string(frame);

        ResourceManager::LoadTexture(
            NumberedTextureName("fire_spell", frame),
            "Src/Assets/Sprites/Fire_Spell/Fire Spell_Frame_" +
                frameNumber + ".png");
    }

    for (int frame = 1; frame <= 10; ++frame)
    {
        ResourceManager::LoadTexture(
            NumberedTextureName("fireball_explosion", frame),
            NumberedTexturePath(
                "Circle_explosion",
                "Circle_explosion",
                frame));
    }

    // The floor fire's own flame animation, cut from the CraftPix "flame1"
    // sequence in Src/Resources/fire.zip. Every frame shares one crop, so the
    // flame stays anchored on its base as the animation plays instead of
    // drifting around inside the quad.
    for (int frame = 1; frame <= GameConfig::FloorFireFrameCount; ++frame)
    {
        ResourceManager::LoadTexture(
            NumberedTextureName("floor_fire", frame),
            NumberedTexturePath("FloorFire", "floor_fire", frame));
    }

    // One white disc, tinted per hazard: red for fire, blue for lightning.
    // White because the sprite shader multiplies texture by tint, so a
    // coloured source can only ever get darker -- which is why the old red
    // warning circle could not simply be re-tinted.
    ResourceManager::LoadTexture(
        "hazard_circle",
        "Src/Assets/Sprites/Warning/soft_circle.png");
}

void Game::AppendLightningSprites(std::vector<Sprite>& frameSprites) const
{
    if (!hazardManager)
        return;

    for (const auto& hazard : hazardManager->GetHazards())
    {
        if (!hazard ||
            hazard->GetMovementPattern() !=
                HazardMovementPattern::WarningThenStrike)
        {
            continue;
        }

        const Hazard* hazardVisual =
            dynamic_cast<const Hazard*>(&hazard->GetVisual());

        if (!hazardVisual ||
            hazardVisual->GetType() != HazardType::Lightning)
        {
            continue;
        }

        const glm::vec3 ground = hazard->GetVisual().GetGroundPosition();

        if (hazard->GetLightningPhase() == LightningPhase::Warning)
        {
            const float warningDuration =
                std::max(hazard->GetWarningDuration(), 0.001f);
            const float warningT = std::clamp(
                hazard->GetPhaseElapsed() / warningDuration,
                0.0f,
                1.0f);

            // Sized from the catch radius the strike actually tests, so the
            // marker on the ground is the danger area rather than an
            // approximation of it: what you see is exactly what you have to
            // be outside of.
            const float diameter = hazard->GetCatchRadius() * 2.0f;

            Sprite warning = Sprite::CreateGroundDecal(
                "hazard_circle",
                ground,
                glm::vec2(diameter, diameter));

            // Blue, because red now belongs to fire. One glance at the floor
            // should say which hazard is about to happen without reading the
            // shape: red ring means burning ground, blue ring means a bolt is
            // coming.
            //
            // This is why the marker moved off red_circle.png and onto the
            // white disc -- the shader multiplies texture by tint, so a red
            // source tinted blue comes out black rather than blue.
            warning.tint = glm::vec3(0.25f, 0.55f, 1.0f);

            // Flashes, and flashes faster as the countdown runs out: a
            // square wave whose period shortens toward zero. Far harder to
            // ignore than a marker that simply brightens, which is the
            // point of a telegraph the player is meant to act on.
            const float period = std::max(
                GameConfig::LightningWarningFlashPeriod * (1.0f - warningT * 0.6f),
                0.05f);

            const float phase = std::fmod(hazard->GetPhaseElapsed(), period);

            const bool bright = phase < period * 0.5f;

            warning.opacity =
                (bright ? 0.70f : 0.28f) * (0.55f + 0.45f * warningT);

            warning.layer = 10;

            frameSprites.push_back(warning);
            continue;
        }

        if (hazard->GetLightningPhase() != LightningPhase::Strike)
            continue;

        // The bolt comes down where the strike resolved: on the player if
        // they were caught, on the marker's centre if they got clear. Fixed
        // at that instant by MovingHazard, so it never chases them
        // afterwards.
        const glm::vec3 strike = hazard->GetStrikePosition();

        const float strikeDuration =
            std::max(hazard->GetStrikeDuration(), 0.001f);
        const float t = std::clamp(
            hazard->GetPhaseElapsed() / strikeDuration,
            0.0f,
            1.0f);

        // The column, anchored at its foot on the strike point and standing
        // up from there, so it reads as having come down onto that spot
        // rather than hovering over it.
        constexpr float BoltHeight = 3.0f;

        const int boltFrame = (t < 0.6f)
            ? std::clamp(1 + static_cast<int>((t / 0.6f) * 5.0f), 1, 5)
            : std::clamp(1 + static_cast<int>(((t - 0.6f) / 0.4f) * 3.0f), 1, 3);

        Sprite bolt = Sprite::CreateBillboard(
            NumberedTextureName(
                t < 0.6f ? "lightning_beginning" : "lightning_end",
                boltFrame),
            strike + glm::vec3(0.0f, BoltHeight * 0.5f, 0.0f),
            glm::vec2(1.0f, BoltHeight));

        // Fades over the tail of the window so it clears rather than
        // vanishing mid-frame.
        bolt.opacity = t < 0.75f ? 1.0f : 1.0f - (t - 0.75f) / 0.25f;
        bolt.layer = 25;

        frameSprites.push_back(bolt);

        // Impact burst on the ground at the same point.
        const int explosionFrame = std::clamp(
            1 + static_cast<int>(t * 10.0f),
            1,
            10);

        Sprite explosion = Sprite::CreateGroundDecal(
            NumberedTextureName("lightning_explosion", explosionFrame),
            strike,
            glm::vec2(
                hazard->GetCatchRadius() * 2.4f,
                hazard->GetCatchRadius() * 2.4f));

        explosion.opacity = 0.9f * (1.0f - t * 0.4f);
        explosion.layer = 20;

        frameSprites.push_back(explosion);
    }
}

void Game::AppendFireballSprites(std::vector<Sprite>& frameSprites) const
{
    if (!hazardManager)
        return;

    for (const auto& hazard : hazardManager->GetHazards())
    {
        if (!hazard)
            continue;

        const Hazard* hazardVisual =
            dynamic_cast<const Hazard*>(&hazard->GetVisual());

        if (!hazardVisual)
            continue;

        const HazardType type = hazardVisual->GetType();

        if (type != HazardType::Fireball && type != HazardType::FloorFire)
            continue;

        const glm::vec3 ground = hazard->GetVisual().GetGroundPosition();

        // The projectile in flight.
        //
        // GetGroundPosition already carries the parabola: UpdateArcProjectile
        // writes the lifted position straight into the visual, so the sprite
        // follows the arc without recomputing any of it here. Movement stays
        // in MovingHazard, drawing stays here.
        if (hazard->GetMovementPattern() ==
            HazardMovementPattern::ArcProjectile)
        {
            const float duration =
                std::max(hazard->GetArcDuration(), 0.001f);
            const float t = std::clamp(
                hazard->GetArcElapsed() / duration,
                0.0f,
                1.0f);

            // Cycled rather than stretched across the flight, so a long lob
            // still reads as a burning ball rather than one slow morph.
            const int frame = 1 + (static_cast<int>(t * 16.0f) % 8);

            const glm::vec3 velocity = hazard->GetVelocity();

            // The ground under the projectile, which the arc's own height is
            // measured from. GetGroundPosition carries the parabola, so it
            // cannot answer this by itself.
            const float groundY = hazard->GetArcGroundHeight();
            const float altitude = std::max(ground.y - groundY, 0.0f);

            //---------------------------------------------------------
            // Shadow, drawn first so the flame sits over it.
            //
            // Tracks X/Z only and stays pinned to the lane surface, which is
            // what makes it read as a shadow rather than a second fireball.
            // It shrinks and fades with altitude, so watching it grow tells
            // the player where and when the fireball is about to land.
            //---------------------------------------------------------
            const float peak =
                std::max(GameConfig::FireballArcHeight, 0.001f);

            const float heightFactor = std::clamp(
                altitude / peak,
                0.0f,
                1.0f);

            const float shadowScale =
                1.0f - GameConfig::FireballShadowHeightFalloff * heightFactor;

            Sprite shadow = Sprite::CreateGroundDecal(
                "hazard_circle",
                glm::vec3(ground.x, groundY, ground.z),
                glm::vec2(
                    GameConfig::FireballShadowSize * shadowScale,
                    GameConfig::FireballShadowSize * shadowScale));

            shadow.tint = glm::vec3(0.0f, 0.0f, 0.0f);
            shadow.opacity = GameConfig::FireballShadowOpacity * shadowScale;
            shadow.layer = 12;

            frameSprites.push_back(shadow);

            //---------------------------------------------------------
            // The projectile.
            //---------------------------------------------------------

            // Gentle breathing so a ball in flight is never a frozen decal.
            const float pulse =
                1.0f + 0.08f * std::sin(t * 18.0f);

            Sprite fireball = Sprite::CreateBillboard(
                NumberedTextureName("fire_spell", frame),
                ground + glm::vec3(0.0f, GameConfig::FireballHitRadius, 0.0f),
                glm::vec2(1.6f * pulse, 0.9f * pulse));

            // The art is drawn nose-left with its tail streaming right, so
            // flipping is what points it the way it is actually going.
            const bool movingRight = velocity.x > 0.0f;

            fireball.flipX = movingRight;

            // ...and this is the part a left/right flip alone cannot do. The
            // path is a parabola, so the fireball climbs and then dives; kept
            // level it visibly flies sideways through its own arc. Aiming the
            // nose along the velocity makes it follow the curve it is on.
            //
            // The nose sits at 180 degrees unflipped and 0 flipped, so the
            // rotation needed is the travel angle minus whichever that is.
            const float travelDegrees = glm::degrees(
                std::atan2(velocity.y, velocity.x));

            fireball.rotationDegrees =
                movingRight ? travelDegrees : travelDegrees - 180.0f;

            fireball.layer = 24;
            frameSprites.push_back(fireball);
            continue;
        }

        if (hazard->GetMovementPattern() ==
            HazardMovementPattern::TemporaryZone)
        {
            const float duration =
                std::max(hazard->GetZoneDuration(), 0.001f);
            const float t = std::clamp(
                hazard->GetZoneElapsed() / duration,
                0.0f,
                1.0f);
            const float elapsed = hazard->GetZoneElapsed();

            //---------------------------------------------------------
            // The red hazard ring.
            //
            // Drawn first, and drawn for the hazard's whole life, because it
            // is the honest statement of where the fire hurts: it is sized
            // straight from the damage radius the collision pass tests. The
            // flame above it is taller and wider than its own footprint, so
            // without this the dangerous area would be guesswork.
            //---------------------------------------------------------
            Sprite ring = Sprite::CreateGroundDecal(
                "hazard_circle",
                ground,
                glm::vec2(
                    GameConfig::FloorFireRingRadius * 2.0f,
                    GameConfig::FloorFireRingRadius * 2.0f));

            ring.tint = glm::vec3(1.0f, 0.12f, 0.05f);

            // Pulses gently, and fades out with the hazard so the warning
            // never outlives the danger it is warning about.
            const float ringPulse = 0.85f + 0.15f * std::sin(elapsed * 7.0f);

            ring.opacity =
                GameConfig::FloorFireRingOpacity * ringPulse * (1.0f - t * 0.5f);

            ring.layer = 14;

            frameSprites.push_back(ring);

            //---------------------------------------------------------
            // The flame.
            //
            // Played out and back over the CraftPix frames rather than
            // looped end-to-start: the sequence grows from a small flame to
            // a full one, so wrapping would snap it large-to-small once a
            // cycle. Bouncing reads as the fire breathing.
            //---------------------------------------------------------
            constexpr int LastFrame = GameConfig::FloorFireFrameCount - 1;

            const int tick = static_cast<int>(
                elapsed * GameConfig::FloorFireFramesPerSecond);

            // Walk 0..last..0 by folding the counter about the end.
            const int cycle = tick % (LastFrame * 2);
            const int step = cycle <= LastFrame
                ? cycle
                : (LastFrame * 2 - cycle);

            const int frame = std::clamp(step + 1, 1, GameConfig::FloorFireFrameCount);

            // Camera-facing rather than flat on the floor: flames stand up.
            // Sat half its own height up so the quad's bottom edge meets the
            // ground it is burning on instead of sinking into it.
            Sprite fire = Sprite::CreateBillboard(
                NumberedTextureName("floor_fire", frame),
                ground + glm::vec3(
                    0.0f,
                    GameConfig::FloorFireSpriteSize * 0.5f,
                    0.0f),
                glm::vec2(
                    GameConfig::FloorFireSpriteSize,
                    GameConfig::FloorFireSpriteSize));

            // Fades out over its lifetime, so the patch going cold is
            // visible before it stops dealing damage.
            fire.opacity = 0.95f * (1.0f - t * 0.35f);
            fire.layer = 22;

            frameSprites.push_back(fire);

            //---------------------------------------------------------
            // Impact flash.
            //
            // Only for the first fraction of a second, and only expanding.
            // It exists to cover the handover: the projectile is destroyed
            // and the fire created on the same frame, and without something
            // bridging them the eye reads a blink rather than a landing.
            //---------------------------------------------------------
            if (elapsed < GameConfig::FireballImpactFlashDuration)
            {
                const float flashT = std::clamp(
                    elapsed / GameConfig::FireballImpactFlashDuration,
                    0.0f,
                    1.0f);

                const float size = glm::mix(
                    GameConfig::FireballImpactFlashStartSize,
                    GameConfig::FireballImpactFlashEndSize,
                    flashT);

                Sprite flash = Sprite::CreateGroundDecal(
                    "hazard_circle",
                    ground,
                    glm::vec2(size, size));

                flash.tint = glm::vec3(1.0f, 0.78f, 0.35f);
                flash.opacity = 0.85f * (1.0f - flashT);
                flash.layer = 19;

                frameSprites.push_back(flash);
            }
        }
    }
}

void Game::TriggerAbilityClearPulse()
{
    if (!pawn)
        return;

    const glm::vec3 origin = pawn->GetTransform().GetPosition();

    AbilityPulse pulse;
    pulse.origin = origin;

    // Whatever is destroyed is handed back here so it can play a death
    // reaction; it is already out of collision and gameplay by then.
    std::vector<std::shared_ptr<GroundEntity>> removedVisuals;

    // Two lists, one rule. Placed props live in stationaryHazards, while the
    // sheep is driven by a MovingHazard, so clearing has to reach into both
    // -- otherwise the sheep would be immune purely because of which
    // container it happens to sit in.
    //
    // Which types either call will accept is decided by IsAbilityClearable,
    // and that is where projectiles are refused.
    const int clearedObstacles = HazardManager::ClearStationaryObstacles(
        stationaryHazards,
        origin,
        GameConfig::BishopClearRadius,
        GameConfig::BishopRemovalCount,
        pulse.clearedPositions,
        removedVisuals);

    // The budget is shared, so a pulse that already used itself up on props
    // does not also get to take a sheep.
    const int remaining = GameConfig::BishopRemovalCount - clearedObstacles;

    if (hazardManager && remaining > 0)
    {
        hazardManager->ClearRemovableHazards(
            origin,
            GameConfig::BishopClearRadius,
            remaining,
            pulse.clearedPositions,
            removedVisuals);
    }

    for (const auto& visual : removedVisuals)
    {
        if (!visual)
            continue;

        DyingObstacle dying;
        dying.visual = visual;
        dying.startGroundPosition = visual->GetGroundPosition();
        dying.startScale = visual->GetTransform().GetScale();

        // The shadow goes immediately. A shadow under a prop that is
        // visibly disintegrating reads as a second object left behind.
        visual->SetShadowVisible(false);

        dyingObstacles.push_back(std::move(dying));
    }

    // Shown even when nothing was in range. A pulse that fires into empty
    // space still tells the player the ability went off and how far it
    // reached, which is more useful than silence that reads as a bug.
    abilityPulses.push_back(std::move(pulse));
}

void Game::UpdateDyingObstacles(float deltaTime)
{
    for (DyingObstacle& dying : dyingObstacles)
    {
        dying.elapsed += deltaTime;

        if (!dying.visual)
            continue;

        const float t = std::clamp(
            dying.elapsed /
                std::max(GameConfig::AbilityDeathReactionDuration, 0.001f),
            0.0f,
            1.0f);

        // Collapses and sinks at the same time, so it reads as being broken
        // apart and swallowed rather than simply scaled away.
        const float scale = glm::mix(
            1.0f,
            GameConfig::AbilityDeathEndScale,
            t);

        dying.visual->GetTransform().SetScale(
            dying.startScale.x * scale,
            dying.startScale.y * scale,
            dying.startScale.z * scale);

        dying.visual->SetGroundPosition(
            dying.startGroundPosition -
            glm::vec3(0.0f, GameConfig::AbilityDeathSinkDistance * t, 0.0f));

        // Flashes toward the pulse's colour on the way out, which is what
        // ties the destruction to the ability that caused it rather than
        // leaving it looking like the prop failed on its own.
        const glm::vec4 base = dying.visual->GetColor();

        dying.visual->SetColor(glm::vec4(
            glm::mix(base.r, 1.0f, t * 0.7f),
            glm::mix(base.g, 0.85f, t * 0.7f),
            glm::mix(base.b, 0.35f, t * 0.7f),
            1.0f - t));
    }

    dyingObstacles.erase(
        std::remove_if(
            dyingObstacles.begin(),
            dyingObstacles.end(),
            [](const DyingObstacle& dying)
            {
                return !dying.visual ||
                    dying.elapsed >= GameConfig::AbilityDeathReactionDuration;
            }),
        dyingObstacles.end());
}

void Game::UpdateAbilityPulses(float deltaTime)
{
    for (AbilityPulse& pulse : abilityPulses)
        pulse.elapsed += deltaTime;

    abilityPulses.erase(
        std::remove_if(
            abilityPulses.begin(),
            abilityPulses.end(),
            [](const AbilityPulse& pulse)
            {
                return pulse.elapsed >= GameConfig::AbilityPulseDuration;
            }),
        abilityPulses.end());
}

void Game::AppendAbilityPulseSprites(std::vector<Sprite>& frameSprites) const
{
    for (const AbilityPulse& pulse : abilityPulses)
    {
        const float t = std::clamp(
            pulse.elapsed / std::max(GameConfig::AbilityPulseDuration, 0.001f),
            0.0f,
            1.0f);

        // The ring: a flat decal growing from the pawn out to the ability's
        // real clear radius, so what the player sees expand is exactly the
        // area that was tested. Fades as it grows.
        const float diameter = glm::mix(
            GameConfig::AbilityPulseStartDiameter,
            GameConfig::AbilityPulseEndDiameter,
            t);

        const int ringFrame = std::clamp(
            1 + static_cast<int>(t * 10.0f),
            1,
            10);

        Sprite ring = Sprite::CreateGroundDecal(
            NumberedTextureName("lightning_explosion", ringFrame),
            pulse.origin,
            glm::vec2(diameter, diameter));

        ring.opacity = 0.75f * (1.0f - t);
        ring.layer = 16;

        frameSprites.push_back(ring);

        // One burst per prop that was actually destroyed. This is the part
        // that answers "what did that just remove?" - the ring alone shows
        // reach, but only these show the result.
        const int burstFrame = std::clamp(
            1 + static_cast<int>(t * 10.0f),
            1,
            10);

        for (const glm::vec3& position : pulse.clearedPositions)
        {
            Sprite burst = Sprite::CreateBillboard(
                NumberedTextureName("fireball_explosion", burstFrame),
                position + glm::vec3(
                    0.0f,
                    GameConfig::AbilityClearBurstSize * 0.5f,
                    0.0f),
                glm::vec2(
                    GameConfig::AbilityClearBurstSize,
                    GameConfig::AbilityClearBurstSize));

            burst.opacity = 1.0f - t * 0.5f;
            burst.layer = 26;

            frameSprites.push_back(burst);
        }
    }
}

void Game::UpdateCamera()
{
    if (!camera || !pawn)
        return;

    const glm::vec3& pawnPosition =
        pawn->GetTransform().GetPosition();

    // X/Z follows gameplay progression; Y follows the ground-centred pawn
    // rather than its jump offset. A jump inside the lock space therefore
    // moves visibly through the frame instead of lifting the whole camera,
    // and the ground-footprint clamp remains stable throughout the arc.
    const glm::vec3 desiredTarget(
        pawnPosition.x,
        pawnPosition.y - pawn->GetJumpHeight(),
        pawnPosition.z - GameConfig::CameraLookAhead);

    glm::vec3 nextTarget;

    if (!cameraFollowInitialized)
    {
        nextTarget = desiredTarget;
        cameraFollowInitialized = true;
    }
    else
    {
        nextTarget = camera->GetTarget();
        nextTarget.y = desiredTarget.y;

        nextTarget.x = FollowOutsideDeadZone(
            nextTarget.x,
            desiredTarget.x,
            GameConfig::CameraDeadZoneWidth);

        nextTarget.z = FollowOutsideDeadZone(
            nextTarget.z,
            desiredTarget.z,
            GameConfig::CameraDeadZoneDepth);
    }

    // Calculate what the camera actually sees, not just where its target is.
    // Setting the candidate first lets Camera3D account for target height in
    // the four corner-ray intersections.
    camera->SetTarget(nextTarget);

    if (level &&
        camera->GetProjectionMode() == ProjectionMode::Orthographic)
    {
        LevelBoundsXZ visualBounds = level->GetBoundaryVisualBounds();

        visualBounds.minX += GameConfig::CameraBoundsInset;
        visualBounds.maxX -= GameConfig::CameraBoundsInset;
        visualBounds.minZ += GameConfig::CameraBoundsInset;
        visualBounds.maxZ -= GameConfig::CameraBoundsInset;

        const CameraGroundBounds viewBounds =
            camera->GetOrthographicGroundBounds(GameConfig::GroundSurface);

        nextTarget.x = FitViewAxis(
            nextTarget.x,
            viewBounds.minX,
            viewBounds.maxX,
            visualBounds.minX,
            visualBounds.maxX);

        nextTarget.z = FitViewAxis(
            nextTarget.z,
            viewBounds.minZ,
            viewBounds.maxZ,
            visualBounds.minZ,
            visualBounds.maxZ);

        camera->SetTarget(nextTarget);
    }
}

bool Game::TryInteractWithCheckpointGate(const glm::vec3& pawnPosition)
{
    for (std::size_t i = 0; i < checkpointGates.size(); ++i)
    {
        const auto& gate = checkpointGates[i];

        if (!gate)
            continue;

        const float distance = glm::length(
            pawnPosition - gate->GetGroundPosition());

        if (distance > GameConfig::InteractRadius)
            continue;

        if (gate->GetDoorAngle() < CheckpointGate::GetMaxDoorAngle())
            checkpointGateOpening[i] = true;

        // Reaching a checkpoint at all activates it, even if E only
        // re-opens an already-open gate: a later respawn should return
        // here, not to whichever checkpoint was activated before it.
        if (pawn)
            pawn->SetSpawnPosition(gate->GetGroundPosition());

        // Standing at the gate at all means E belongs to it, not the
        // ability -- even if it's already fully open and there's nothing
        // left to start.
        return true;
    }

    return false;
}

bool Game::TryInteractWithKingsCage(const glm::vec3& pawnPosition)
{
    if (!kingsCage ||
        !kingsCage->GetLeftDoor() ||
        !kingsCage->GetRightDoor())
        return false;

    const float distance = glm::length(
        pawnPosition - kingsCage->GetGroundPosition());

    if (distance > GameConfig::InteractRadius)
        return false;

    if (kingsCageDoorAngle < GameConfig::KingsCageMaxDoorAngle)
        kingsCageDoorOpening = true;

    return true;
}

void Game::UpdateDoors(float deltaTime)
{
    for (std::size_t i = 0; i < checkpointGates.size(); ++i)
    {
        if (!checkpointGateOpening[i] || !checkpointGates[i])
            continue;

        const float newAngle = std::min(
            checkpointGates[i]->GetDoorAngle() +
                GameConfig::DoorOpenSpeed * deltaTime,
            CheckpointGate::GetMaxDoorAngle());

        checkpointGates[i]->SetDoorAngle(newAngle);

        if (newAngle >= CheckpointGate::GetMaxDoorAngle())
            checkpointGateOpening[i] = false;
    }

    if (kingsCageDoorOpening && kingsCage)
    {
        WorldObject* leftDoor = kingsCage->GetLeftDoor();
        WorldObject* rightDoor = kingsCage->GetRightDoor();

        if (leftDoor && rightDoor)
        {
            kingsCageDoorAngle = std::min(
                kingsCageDoorAngle + GameConfig::DoorOpenSpeed * deltaTime,
                GameConfig::KingsCageMaxDoorAngle);

            // The entrance faces +Z. The left leaf extends +X from its outer
            // hinge, so negative Y sends its free edge toward +Z; the right
            // leaf extends -X and needs the opposite sign. Both therefore
            // swing outward, away from the cage interior at -Z.
            leftDoor->GetTransform().SetRotation(
                0.0f, -kingsCageDoorAngle, 0.0f);

            rightDoor->GetTransform().SetRotation(
                0.0f, kingsCageDoorAngle, 0.0f);

            if (kingsCageDoorAngle >= GameConfig::KingsCageMaxDoorAngle)
                kingsCageDoorOpening = false;
        }
    }
}

void Game::Update(float deltaTime)
{
    if (level)
        level->Update(deltaTime);

    // Before the pawn's own update, so a character switched this frame is
    // the one that moves and animates this frame.
    if (pawn && pieceMeshes)
        cheats.Update(*pawn, *pieceMeshes);

    if (pawn && pawn->IsActive())
        pawn->Update(deltaTime);

    if (collectibleManager)
    {
        for (const auto& piece : collectibleManager->GetCollectibles())
        {
            if (piece && piece->IsActive())
                piece->Update(deltaTime);
        }
    }

    if (hazardManager && pawn)
        hazardManager->Update(deltaTime, pawn->GetTransform().GetPosition());

    // ---- Update Hazard Collision ----
    if (hazardCollision)
    {
        hazardCollision->Update(
            hazardManager->GetHazards(),
            stationaryHazards,
            deltaTime);

        if (kingsCage)
        {
            std::vector<CollisionBox> cageBoxes;
            cageBoxes.push_back(kingsCage->GetCollisionBox());
            hazardCollision->BlockAgainstBoxes(cageBoxes);
        }

        // The checkpoint gates are structures, not hazards: their walls and
        // leaves only need to be solid. Rebuilt from the gates every frame so
        // each gate's leaves follow its own swing, which is what keeps the
        // collision and the opening animation in step.
        if (!checkpointGates.empty())
        {
            gateCollisionBoxes.clear();

            for (const auto& gate : checkpointGates)
            {
                if (gate)
                    gate->AppendCollisionBoxes(gateCollisionBoxes);
            }

            hazardCollision->BlockAgainstBoxes(gateCollisionBoxes);
        }
    }

    // TEMPORARY: stands in for Kaung's real collision detection.
    if (collectibleManager && pawn)
    {
        PieceType collectedType = PieceType::Pawn;

        if (collectibleManager->TryCollect(
            pawn->GetTransform().GetPosition(),
            GameConfig::CollectiblePickupRadius,
            collectedType))
        {
            pawn->CollectPiece(collectedType);
        }
    }

    // Bishop's ability mutates the world by breaking down the stationary
    // props around the pawn. The Queen raises the same pulse, so she inherits
    // this behaviour exactly rather than having a second copy of it.
    //
    // Moving hazards are deliberately untouched: an arrow already in flight
    // stays in flight.
    if (pawn && pawn->ConsumeBishopActivationPulse())
        TriggerAbilityClearPulse();

    UpdateAbilityPulses(deltaTime);
    UpdateDyingObstacles(deltaTime);

    // E's target isn't decided inside Pawn (see ConsumeInteractPulse) --
    // resolve it here against whatever's actually in the level: the
    // checkpoint gate, then the king's cage door, then finally the pawn's
    // own banked ability if neither was in range.
    if (pawn && pawn->ConsumeInteractPulse())
    {
        const glm::vec3 pawnPosition = pawn->GetTransform().GetPosition();

        const bool interactedWithWorld =
            TryInteractWithCheckpointGate(pawnPosition) ||
            TryInteractWithKingsCage(pawnPosition);

        if (!interactedWithWorld)
            pawn->TryActivateAbility();
    }

    UpdateDoors(deltaTime);

    UpdateCamera();
}

void Game::DrawPiece(const ChessPiece& piece)
{
    if (!renderer)
        return;

    // A rigged piece is several meshes carrying their own transforms; an
    // unrigged one is a single baked mesh. Both end up in the same Draw, so
    // nothing downstream of here knows or cares which it is looking at.
    const auto& parts = piece.GetRigParts();

    if (parts.empty())
    {
        renderer->Draw(piece);
        return;
    }

    for (const auto& part : parts)
    {
        if (part)
            renderer->Draw(*part);
    }
}

void Game::DrawPawn()
{
    if (!renderer || !pawn)
        return;

    const auto& parts = pawn->GetRigParts();

    if (parts.empty())
    {
        renderer->Draw(*pawn);
        return;
    }

    for (const auto& part : parts)
    {
        if (part)
            renderer->Draw(*part);
    }
}

void Game::Render()
{
    if (!renderer)
        return;

    // Ground first, then everything standing on it. The depth buffer sorts
    // the solid geometry, so this order only matters for the shadow, which
    // is translucent and has to blend over ground that is already drawn.
    if (level)
    {
        for (const auto& lane : level->GetLanes())
        {
            if (lane)
                renderer->Draw(*lane);
        }

        for (const auto& decoration : level->GetDecorations())
        {
            if (decoration)
                renderer->Draw(*decoration);
        }
    }

    // ---- SHADOWS ----
    // Every shadow before every model: shadows are translucent and have to
    // blend over ground that has already been drawn.
    for (const auto& hazard : stationaryHazards)
    {
        if (hazard)
            renderer->Draw(hazard->GetShadow());
    }

    for (const auto& gate : checkpointGates)
    {
        if (gate)
            renderer->Draw(gate->GetShadow());
    }

    if (capturedKing)
        renderer->Draw(capturedKing->GetShadow());

    if (hazardManager)
    {
        for (const auto& hazard : hazardManager->GetHazards())
        {
            if (hazard)
                renderer->Draw(hazard->GetVisual().GetShadow());
        }
    }

    if (collectibleManager)
    {
        for (const auto& piece : collectibleManager->GetCollectibles())
        {
            if (piece)
                renderer->Draw(piece->GetShadow());
        }
    }

    if (pawn)
        renderer->Draw(pawn->GetShadow());

    // ---- MODELS ----

    // Each gate is several meshes rather than one, because its two leaves
    // turn on their own hinges.
    for (const auto& gate : checkpointGates)
    {
        if (!gate)
            continue;

        for (const auto& part : gate->GetParts())
        {
            if (part)
                renderer->Draw(*part);
        }
    }

    // The frame is the cage's root mesh. Each barred leaf is a separate
    // object with its origin on its outer hinge.
    if (kingsCage)
        renderer->Draw(*kingsCage);

    // Through DrawPiece, not the renderer directly. The King is an animated
    // piece, and an animated piece has no baked mesh of its own - it draws
    // as a hierarchy of rig parts. Drawing it as a plain WorldObject asked
    // the renderer for a null mesh, which it quietly skips, and the prisoner
    // simply never appeared inside his cage.
    if (capturedKing)
        DrawPiece(*capturedKing);

    if (kingsCage)
    {
        if (WorldObject* leftDoor = kingsCage->GetLeftDoor())
            renderer->Draw(*leftDoor);

        if (WorldObject* rightDoor = kingsCage->GetRightDoor())
            renderer->Draw(*rightDoor);
    }

    for (const auto& hazard : stationaryHazards)
    {
        if (hazard)
            renderer->Draw(*hazard);
    }

    // Props the Bishop destroyed, still collapsing. Drawn with the rest of
    // the world because that is what they still are for another moment --
    // they are simply no longer in anything that can be collided with.
    for (const DyingObstacle& dying : dyingObstacles)
    {
        if (dying.visual)
            renderer->Draw(*dying.visual);
    }

    if (hazardManager)
    {
        for (const auto& hazard : hazardManager->GetHazards())
        {
            if (hazard)
                renderer->Draw(hazard->GetVisual());
        }
    }

    if (collectibleManager)
    {
        for (const auto& piece : collectibleManager->GetCollectibles())
            DrawPiece(*piece);
    }

    if (pawn)
        DrawPawn();

    // Sprites last, after every opaque mesh.
    //
    // They are transparent and do not write depth, so anything drawn after
    // them would ignore them and punch straight through. Persistent sprites
    // live in `sprites`; hazard VFX are rebuilt into this frame-local list
    // from their timing state.
    std::vector<Sprite> frameSprites = sprites;
    AppendLightningSprites(frameSprites);
    AppendFireballSprites(frameSprites);
    AppendAbilityPulseSprites(frameSprites);

    if (spriteRenderer)
        spriteRenderer->DrawAll(frameSprites);
}

std::size_t Game::AddSprite(const Sprite& sprite)
{
    sprites.push_back(sprite);

    return sprites.size() - 1;
}

Sprite* Game::GetSprite(std::size_t index)
{
    if (index >= sprites.size())
        return nullptr;

    return &sprites[index];
}

void Game::RemoveSprite(std::size_t index)
{
    if (index >= sprites.size())
        return;

    sprites.erase(sprites.begin() + static_cast<std::ptrdiff_t>(index));
}

void Game::ClearSprites()
{
    sprites.clear();
}

std::size_t Game::GetSpriteCount() const
{
    return sprites.size();
}

void Game::SetViewportSize(float width, float height)
{
    if (camera)
        camera->SetViewport(width, height);

    if (spriteRenderer)
        spriteRenderer->SetScreenSize(width, height);

    // Aspect ratio changes the orthographic ground footprint, so the target
    // must be re-fitted immediately rather than waiting for player movement.
    UpdateCamera();
}

void Game::Shutdown()
{
    sprites.clear();

    // Before the resource manager, because the renderer's shader and quad are
    // GL objects that must go while the context is current.
    if (spriteRenderer)
    {
        spriteRenderer->Shutdown();
        spriteRenderer.reset();
    }

    // Props before the libraries: both hold the shared meshes, and every GPU
    // buffer has to be released while the GL context is still alive.
    stationaryHazards.clear();
    capturedKing.reset();
    kingsCage.reset();
    checkpointGates.clear();

    if (hazardManager)
    {
        hazardManager->Clear();
        hazardManager.reset();
    }

    if (collectibleManager)
    {
        collectibleManager->Clear();
        collectibleManager.reset();
    }

    hazardCollision.reset();

    pieceMeshes.reset();
    obstacleMeshes.reset();
    cageMeshes.reset();
    gateMeshes.reset();

    pawn.reset();
    level.reset();
    renderer.reset();
    camera.reset();
    cameraFollowInitialized = false;

    ResourceManager::Shutdown();
}

Camera3D& Game::GetCamera()
{
    return *camera;
}

MeshRenderer& Game::GetRenderer()
{
    return *renderer;
}

Level& Game::GetLevel()
{
    return *level;
}

Pawn& Game::GetPawn()
{
    return *pawn;
}

PieceMeshLibrary& Game::GetPieceMeshes()
{
    return *pieceMeshes;
}

ObstacleMeshLibrary& Game::GetObstacleMeshes()
{
    return *obstacleMeshes;
}

CheckpointGate* Game::GetCheckpointGate()
{
    return checkpointGates.empty() ? nullptr : checkpointGates.front().get();
}

const std::vector<std::shared_ptr<CheckpointGate>>& Game::GetCheckpointGates() const
{
    return checkpointGates;
}

KingsCage* Game::GetKingsCage()
{
    return kingsCage.get();
}

ChessPiece* Game::GetCapturedKing()
{
    return capturedKing.get();
}

HazardManager& Game::GetHazardManager()
{
    return *hazardManager;
}

CollectibleManager& Game::GetCollectibleManager()
{
    return *collectibleManager;
}

SpriteRenderer& Game::GetSpriteRenderer()
{
    return *spriteRenderer;
}