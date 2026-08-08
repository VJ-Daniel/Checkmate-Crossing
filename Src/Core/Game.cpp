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
#include <unordered_map>

#include "GameConfig.h"
#include "ResourceManager.h"
#include "SceneNode.h"
#include "Shader.h"
#include "AudioManager.h"
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

    AudioManager::Initialize();
    AudioManager::PlayMusic("Src/Resources/Sounds/bg_music.mp3");



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

    hudScreenWidth = screenWidth;
    hudScreenHeight = screenHeight;

    controlsScreenTimer = GameConfig::ControlsScreenDuration;

    // 1. Create the libraries FIRST before anything else uses them.
    pieceMeshes = std::make_shared<PieceMeshLibrary>();

    obstacleMeshes = std::make_shared<ObstacleMeshLibrary>();
    gateMeshes = std::make_shared<GateMeshLibrary>();
    cageMeshes = std::make_shared<CageMeshLibrary>();

    LoadLightningSprites();
    LoadFireballSprites();
    LoadHudSprites();

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
    checkpointGateActivated.clear();

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
        checkpointGateActivated.push_back(false);
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
    // How far each piece is set into its neighbour.
    //
    // Small and deliberate: enough that the seam between two models closes
    // instead of merely meeting, without the two visibly interpenetrating.
    constexpr float BarrierOverlap = 0.06f;

    // Every piece's own width, so a barrier's spacing comes from the models
    // in it rather than a number typed at the call site. That is what was
    // wrong before: walls were stepped 1.10 apart with a 1.02-wide model and
    // fences 1.05 with a 0.98-wide one, so every barrier in the level was a
    // row of separate props with a finger's width of daylight between them
    // rather than one structure.
    const auto modelWidth = [this](ObstacleType type)
        {
            return obstacleMeshes->GetModel(type).footprintWidth;
        };

    auto placeBarrier = [&place, &modelWidth, halfWidth](
        const Lane& lane,
        ObstacleType type,
        float gapCenterX,
        float gapHalfWidth)
        {
            const float width = modelWidth(type);
            const float step = std::max(width - BarrierOverlap, 0.1f);

            // Runs edge to edge, with the last piece pulled flush against the
            // far boundary rather than stopping wherever the stride happened
            // to run out. That remainder used to be left open: a barrier
            // covered the field until roughly x = +3.8 and left a slot
            // against the wall, which on three rows was wide enough to walk
            // round the whole thing.
            for (float x = -halfWidth + width * 0.5f; ; x += step)
            {
                const bool isLast = x > halfWidth - width * 0.5f;

                const float center =
                    isLast ? (halfWidth - width * 0.5f) : x;

                if (std::fabs(center - gapCenterX) >= gapHalfWidth)
                    place(type, center, lane);

                if (isLast)
                    break;
            }
        };

    // Same idea, but alternating two prop types across the span so a barrier
    // reads as something assembled out of whatever was to hand.
    //
    // Advances by whichever piece was just placed rather than by one shared
    // step, since the two types are rarely the same width - stepping by a
    // single figure leaves a seam after every narrower piece.
    auto placeMixedBarrier = [&place, &modelWidth, halfWidth](
        const Lane& lane,
        ObstacleType first,
        ObstacleType second,
        float gapCenterX,
        float gapHalfWidth)
        {
            int index = 0;

            // Left edge of the next piece to be laid.
            float cursor = -halfWidth;

            for (;;)
            {
                const ObstacleType type =
                    (index % 2 == 0) ? first : second;

                const float width = modelWidth(type);

                const bool isLast =
                    cursor + width * 0.5f > halfWidth - width * 0.5f;

                // Same edge rule as the single-type barrier: the final piece
                // is set flush to the boundary so no slot is left against it.
                const float center = isLast
                    ? (halfWidth - width * 0.5f)
                    : (cursor + width * 0.5f);

                // Skipped for the opening, but the cursor still advances, so
                // the pieces past the gap stay in line with those before it.
                if (std::fabs(center - gapCenterX) >= gapHalfWidth)
                    place(type, center, lane);

                if (isLast)
                    break;

                cursor += std::max(width - BarrierOverlap, 0.1f);
                ++index;
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
                    halfWidth * 0.58f, GapHalfWidth);
            }

            if (rows.size() >= 2)
            {
                // Trees and rocks together: neither can be broken, so this
                // row is purely about finding the line through it.
                placeMixedBarrier(
                    *rows[1], ObstacleType::Tree, ObstacleType::Rock,
                    -halfWidth * 0.42f, GapHalfWidth);
            }

            if (rows.size() >= 3)
            {
                // Walls neither break nor jump (GDD), so this is the row
                // that genuinely cannot be forced - the gap is the only way.
                placeBarrier(
                    *rows[2], ObstacleType::Wall,
                    halfWidth * 0.15f, GapHalfWidth);
            }

            if (rows.size() >= 4)
            {
                // Palisade and fence, both of which stop the player.
                //
                // This row used to alternate palisade with bush, and a bush
                // does not block - it only slows. Every second piece was
                // therefore a hole, and the row read as a barrier while
                // being walked straight through in five places. Bushes are
                // scenery, and belong on the open grass rather than in a
                // structure meant to turn the player.
                placeMixedBarrier(
                    *rows[3], ObstacleType::Palisade, ObstacleType::Fence,
                    -halfWidth * 0.66f, GapHalfWidth);
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
                    halfWidth * 0.70f, GapHalfWidth);
            }

            if (rows.size() >= 2)
            {
                placeMixedBarrier(
                    *rows[1], ObstacleType::Fence, ObstacleType::Tree,
                    -halfWidth * 0.22f, GapHalfWidth);
            }

            if (rows.size() >= 3)
            {
                placeMixedBarrier(
                    *rows[2], ObstacleType::Wall, ObstacleType::Fence,
                    halfWidth * 0.40f, GapHalfWidth);
            }
        }
    }

    //-----------------------------------------------------------
    // Collectible stashes.
    //
    // Props framing each collectible ally, so a piece reads as something
    // stowed in a defended spot rather than dropped in a gap. Deliberately
    // pairs and singles, never a barrier: each leaves its row open on both
    // sides, so a player who does not want the piece walks past without ever
    // being funnelled. The positions match BuildLevelCollectibles.
    //-----------------------------------------------------------
    {
        const auto laneAt =
            [](const std::vector<const Lane*>& rows, std::size_t index)
            -> const Lane*
            {
                return index < rows.size() ? rows[index] : nullptr;
            };

        const auto arrowRuns = FindRunsOfType(*level, LaneType::Arrow);
        const auto spikeRuns = FindRunsOfType(*level, LaneType::SpikeMud);
        const auto cannonRuns = FindRunsOfType(*level, LaneType::Cannonball);
        const auto grassRuns = FindRunsOfType(*level, LaneType::SafeGrass);

        // Bishop's nook: two stakes making an alcove off the side of the
        // spear row, so the piece is tucked out of the through-line.
        if (arrowRuns.size() >= 1)
        {
            if (const Lane* row = laneAt(arrowRuns[0], 1))
            {
                place(ObstacleType::Palisade, -4.10f, *row);
                place(ObstacleType::Palisade, -1.50f, *row);
            }
        }

        // Knight's screen: a rock on the row in front, so the piece cannot
        // be taken head-on in a straight line - the approach has to come in
        // from the side, which is the side the arrow sweeps from.
        if (spikeRuns.size() >= 1)
        {
            if (const Lane* row = laneAt(spikeRuns[0], 0))
                place(ObstacleType::Rock, 0.20f, *row);
        }

        // Rook's pocket: fences either side inside the cannonball lane. They
        // do not stop the cannonball, which is the point - they mark the
        // spot and take away the room to stroll out sideways.
        if (cannonRuns.size() >= 1)
        {
            if (const Lane* row = laneAt(cannonRuns[0], 1))
            {
                place(ObstacleType::Fence, -4.00f, *row);
                place(ObstacleType::Fence, -1.70f, *row);
            }
        }

        // The Queen's alley: two walls well clear of both spear impacts,
        // closing off the sideways escape from the crossfire row and framing
        // the band she sits in.
        if (grassRuns.size() >= 8)
        {
            if (const Lane* row = laneAt(grassRuns[7], 0))
            {
                place(ObstacleType::Wall, -2.70f, *row);
                place(ObstacleType::Wall, 4.00f, *row);
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

    //-----------------------------------------------------------
    // Projectile lanes.
    //
    // Level 1 used to carry one arrow field, one spear and one cannonball,
    // which meant most of it was crossed by waiting for a single hazard to
    // pass. The three types are now spread the length of the level, come
    // from both sides, and run on intervals that do not divide into each
    // other, so a rhythm learned in one row does not carry to the next.
    //
    // Two rules keep it fair:
    //
    //   - Nothing sweeps a barrier row. Those rows funnel the player into a
    //     single gap, and a projectile crossing one is not a dodge, it is a
    //     toll. The woodland rows carry the cow and nothing else.
    //   - Every lane past the first is given a starting offset. Lanes that
    //     all fire on registration arrive as one wall, and stay synchronised
    //     wherever their intervals line up.
    //
    // Nothing is placed in the finale or past the final checkpoint; those
    // belong to fireballs and lightning, and to the walk up to the King.
    //-----------------------------------------------------------

    // Row of a run, guarded so a shorter level cannot index past the end.
    const auto laneAt =
        [](const std::vector<const Lane*>& rows, std::size_t index) -> const Lane*
        {
            return index < rows.size() ? rows[index] : nullptr;
        };

    const auto arrowRuns = FindRunsOfType(*level, LaneType::Arrow);
    const auto spikeRuns = FindRunsOfType(*level, LaneType::SpikeMud);
    const auto cannonRuns = FindRunsOfType(*level, LaneType::Cannonball);
    const auto grassRuns = FindRunsOfType(*level, LaneType::SafeGrass);

    const auto onLane = [&](const Lane* target, auto&& place)
        {
            if (target)
                place(target->GetSurfaceHeight(), target->GetCenterZ());
        };

    //--- First arrow field: the teaching ground -----------------
    if (arrowRuns.size() >= 1)
    {
        const auto& rows = arrowRuns[0];

        // Two arrows running opposite ways with a spear row between them.
        // The middle row is the shelter, and the spear is what stops it
        // being a free one.
        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterArrowLane(y, z, true, 3.0f); });

        onLane(laneAt(rows, 1), [&](float y, float z)
            { RegisterSpearVolley(y, z, true, 3.6f, 0.0f, 0.0f); });

        onLane(laneAt(rows, 2), [&](float y, float z)
            { RegisterArrowLane(y, z, false, 3.4f); });
    }

    //--- Connector into the spike field -------------------------
    if (grassRuns.size() >= 2)
    {
        // A spear from the far side, so the first thing thrown from the
        // right arrives before the player has settled into dodging left.
        onLane(laneAt(grassRuns[1], 0), [&](float y, float z)
            { RegisterSpearVolley(y, z, false, 4.2f, 1.4f, 0.5f); });
    }

    //--- Spike field: one slow arrow across the middle ----------
    if (spikeRuns.size() >= 1)
    {
        // Slower than the open-ground arrows on purpose. The spikes already
        // limit where the player can stand, so the crossing window has to be
        // longer to stay honest.
        onLane(laneAt(spikeRuns[0], 1), [&](float y, float z)
            { RegisterArrowLane(y, z, true, 2.6f); });
    }

    //--- Cannonball section and its approach --------------------
    if (grassRuns.size() >= 4)
    {
        onLane(laneAt(grassRuns[3], 0), [&](float y, float z)
            { RegisterCannonballLane(y, z, true, 3.1f, 0.7f, halfWidth * 0.70f); });
    }

    if (cannonRuns.size() >= 1)
    {
        const auto& rows = cannonRuns[0];

        // Logs roll down the length of this section and out the far side,
        // two lanes on opposite sides of the corridor and out of step.
        //
        // There was one, and it ran from the section's first row to its last
        // - three rows, two world units. At 2.2 a unit a second that is a log
        // alive for under a second in every three, covering a stretch barely
        // longer than the log itself, which is why they read as missing
        // rather than as a hazard. They now run the section and the open
        // ground either side of it.
        if (!rows.empty() && rows.front() && rows.back())
        {
            const float y = rows.front()->GetSurfaceHeight();

            // Two rows before the section and two past it, so a log is
            // already rolling when the player reaches the first cannonball
            // row and is still going as they leave the last.
            const float startZ = rows.back()->GetCenterZ() - 2.0f;
            const float endZ = rows.front()->GetCenterZ() + 2.0f;

            RegisterRollingLogLane(y, halfWidth * 0.44f, startZ, endZ, 3.4f, 0.0f);

            // Left lane set just inside the Rook's fenced pocket rather than
            // through it. Running it wider crashed the log into that pocket's
            // own fence a third of the way down, and running it through would
            // have put a second timed threat inside a stash that already has
            // to be taken between cannonballs.
            RegisterRollingLogLane(y, -0.55f, startZ, endZ, 4.1f, 1.7f);
        }

        // "Faster than arrows... range is shorter and reaches about 70% of
        // the map's width." Firing from opposite edges on different clocks,
        // so neither side of the section is reliably safe.
        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterCannonballLane(y, z, false, 2.6f, 0.0f, halfWidth * 0.70f); });

        onLane(laneAt(rows, 1), [&](float y, float z)
            { RegisterCannonballLane(y, z, true, 2.9f, 1.3f, halfWidth * 0.85f); });

        onLane(laneAt(rows, 2), [&](float y, float z)
            { RegisterArrowLane(y, z, false, 3.2f); });
    }

    //--- Connector before the first woodland --------------------
    if (grassRuns.size() >= 5)
    {
        const auto& rows = grassRuns[4];

        // Both sides, different landing spots, well out of step. This is the
        // clearest statement of what a spear is: two arcs crossing over one
        // stretch of ground, each dangerous only where it comes down.
        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterSpearVolley(y, z, true, 3.8f, 0.5f, -1.0f); });

        onLane(laneAt(rows, 1), [&](float y, float z)
            { RegisterSpearVolley(y, z, false, 4.4f, 2.2f, 0.8f); });
    }

    // The woodland rows themselves stay clear: barriers plus a cow is
    // already a full row, and a sweep across a one-gap barrier would be
    // unavoidable rather than hard.

    //--- After the second checkpoint ----------------------------
    if (grassRuns.size() >= 7)
    {
        onLane(laneAt(grassRuns[6], 0), [&](float y, float z)
            { RegisterCannonballLane(y, z, false, 3.3f, 0.9f, halfWidth * 0.75f); });
    }

    //--- Second arrow field: faster, both ways ------------------
    if (arrowRuns.size() >= 2)
    {
        const auto& rows = arrowRuns[1];

        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterArrowLane(y, z, true, 3.6f); });

        onLane(laneAt(rows, 1), [&](float y, float z)
            { RegisterArrowLane(y, z, false, 3.9f); });
    }

    //--- The crossfire row --------------------------------------
    if (grassRuns.size() >= 8)
    {
        const auto& rows = grassRuns[7];

        // Two throwers on one row, opposite sides, landing apart and timed
        // apart, so there is always ground to stand on between them.
        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterSpearVolley(y, z, false, 3.0f, 0.4f, 0.0f); });

        onLane(laneAt(rows, 0), [&](float y, float z)
            { RegisterSpearVolley(y, z, true, 3.4f, 1.9f, 1.5f); });
    }

    //--- Second spike field -------------------------------------
    if (spikeRuns.size() >= 2)
    {
        onLane(laneAt(spikeRuns[1], 1), [&](float y, float z)
            { RegisterCannonballLane(y, z, true, 3.5f, 1.6f, halfWidth * 0.60f); });
    }

    //--- Last open row before the second woodland ---------------
    if (grassRuns.size() >= 9)
    {
        onLane(laneAt(grassRuns[8], 0), [&](float y, float z)
            { RegisterArrowLane(y, z, true, 4.2f); });
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
    // level, reading as the battlefield collapsing in behind the player.
    //
    // Runs the open corridor between the two woodlands rather than starting
    // on a woodland row. Boulders are now stopped by anything solid, and a
    // gauntlet that began inside a barrier would have every rock die on the
    // frame it spawned. Ending short of the checkpoint keeps them off its
    // gate, which is solid too.
    //
    // The lanes are set clear of the walls flanking the Queen so a rock's run
    // is decided by the level rather than by one prop, but nothing depends on
    // that: a boulder that does meet something solid simply stops there.
    {
        const auto woodlandRuns = FindRunsOfType(*level, LaneType::FenceTree);
        const auto spikeRuns = FindRunsOfType(*level, LaneType::SpikeMud);

        // From the last spike field down to the row after the first woodland.
        const Lane* startLane =
            (spikeRuns.size() < 2 || spikeRuns.back().empty())
            ? nullptr
            : spikeRuns.back().front();

        const Lane* endLane =
            (woodlandRuns.empty() || woodlandRuns.front().empty())
            ? nullptr
            : level->GetLane(woodlandRuns.front().back()->GetRow() + 1);

        if (startLane && endLane)
        {
            const float y = startLane->GetSurfaceHeight();
            const float startZ = startLane->GetCenterZ();
            const float endZ = endLane->GetCenterZ();

            const float lanesX[3] = { -1.40f, 0.60f, 2.60f };

            float delay = 0.0f;

            for (float x : lanesX)
            {
                hazardManager->RegisterRepeatingSpawn(
                    5.0f,
                    [this, y, x, startZ, endZ]()
                    {
                        hazardManager->SpawnLinearHazard(
                            HazardType::RollingRock,
                            glm::vec3(x, y, startZ),
                            glm::vec3(x, y, endZ),
                            1.2f,
                            false);
                    },
                    delay);

                delay += 1.7f;
            }
        }
    }

    // A second pair of logs, rolling the stretch the boulders share.
    //
    // Different axis of threat to everything else in that corridor: the
    // arrows sweep it across, the spears drop into it, and these run down it,
    // so the section cannot be read as one repeated pattern.
    {
        const auto spikeRuns = FindRunsOfType(*level, LaneType::SpikeMud);
        const auto grassRuns = FindRunsOfType(*level, LaneType::SafeGrass);

        const Lane* startLane =
            (spikeRuns.size() < 2 || spikeRuns.back().empty())
            ? nullptr
            : spikeRuns.back().back();

        const Lane* endLane =
            (grassRuns.size() < 7 || grassRuns[6].empty())
            ? nullptr
            : grassRuns[6].front();

        if (startLane && endLane)
        {
            const float y = startLane->GetSurfaceHeight();
            const float startZ = startLane->GetCenterZ();
            const float endZ = endLane->GetCenterZ();

            // Lanes threaded between the boulder lanes and clear of the
            // walls flanking the Queen.
            RegisterRollingLogLane(y, -0.60f, startZ, endZ, 3.7f, 0.9f);
            RegisterRollingLogLane(y, 1.60f, startZ, endZ, 4.3f, 2.6f);
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

void Game::RegisterArrowLane(
    float surfaceHeight,
    float laneZ,
    bool fromLeft,
    float speed)
{
    if (!hazardManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();
    const float edge = halfWidth + 0.5f;

    const glm::vec3 start(fromLeft ? -edge : edge, surfaceHeight, laneZ);
    const glm::vec3 end(fromLeft ? edge : -edge, surfaceHeight, laneZ);

    // Loops on its own, so it needs no repeating spawner.
    hazardManager->SpawnLinearHazard(
        HazardType::Arrow, start, end, speed, true);
}

void Game::RegisterSpearVolley(
    float surfaceHeight,
    float laneZ,
    bool fromLeft,
    float interval,
    float initialDelay,
    float landingOffset)
{
    if (!hazardManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    const float edgeX = halfWidth + GameConfig::SpearSpawnMargin;
    const float startX = fromLeft ? -edgeX : edgeX;

    // Lands inside the field rather than crossing it: travel distance from
    // the throwing edge, shifted by the caller's offset so two throwers on
    // one row do not both hit the same spot.
    const float rawLandingX = fromLeft
        ? startX + GameConfig::SpearTravelDistance + landingOffset
        : startX - GameConfig::SpearTravelDistance - landingOffset;

    // Kept inside the playable width, so the danger area it leaves is always
    // somewhere the player can actually be standing.
    const float landingX = std::clamp(
        rawLandingX,
        -halfWidth + GameConfig::SpearImpactRadius,
        halfWidth - GameConfig::SpearImpactRadius);

    // Both ends share laneZ, so the arc rises and falls without leaving the
    // row it was thrown along.
    const glm::vec3 start(startX, surfaceHeight, laneZ);
    const glm::vec3 landing(landingX, surfaceHeight, laneZ);

    hazardManager->RegisterRepeatingSpawn(
        interval,
        [this, start, landing]()
        {
            hazardManager->SpawnArcHazard(
                HazardType::Spear,
                start,
                landing,
                GameConfig::SpearTravelDuration,
                GameConfig::SpearArcHeight);
        },
        initialDelay);
}

void Game::RegisterCannonballLane(
    float surfaceHeight,
    float laneZ,
    bool fromLeft,
    float interval,
    float initialDelay,
    float reach)
{
    if (!hazardManager)
        return;

    const glm::vec3 start(fromLeft ? -reach : reach, surfaceHeight, laneZ);
    const glm::vec3 end(fromLeft ? reach : -reach, surfaceHeight, laneZ);

    hazardManager->RegisterRepeatingSpawn(
        interval,
        [this, start, end]()
        {
            hazardManager->SpawnLinearHazard(
                HazardType::Cannonball, start, end, 5.5f, false);
        },
        initialDelay);
}

void Game::RegisterRollingLogLane(
    float surfaceHeight,
    float laneX,
    float startZ,
    float endZ,
    float interval,
    float initialDelay)
{
    if (!hazardManager)
        return;

    const glm::vec3 start(laneX, surfaceHeight, startZ);
    const glm::vec3 end(laneX, surfaceHeight, endZ);

    hazardManager->RegisterRepeatingSpawn(
        interval,
        [this, start, end]()
        {
            // Faster and with a smaller hit area than a boulder, per the GDD.
            hazardManager->SpawnLinearHazard(
                HazardType::RollingLog, start, end, 2.6f, false);
        },
        initialDelay);
}

void Game::BuildLevelCollectibles()
{
    if (!level || !collectibleManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    //-----------------------------------------------------------
    // Collectible allies.
    //
    // Each one is stashed inside a hazard section rather than left on the
    // safe ground between them. They used to sit on open grass, and the
    // Rook was on the checkpoint row itself - all four were picked up by
    // walking forward, which is not a decision.
    //
    // Every position below is off the straight line down the level, so
    // ignoring a piece costs nothing and going for one is a choice. None of
    // them is placed inside an obstacle, inside a spear's landing zone, or
    // in a spot that can only be reached by eating a hit; BuildLevelObstacles
    // frames each with props to make it read as a stash rather than a piece
    // dropped in a gap.
    //
    // Difficulty climbs with the level: an arrow field, then a spike field,
    // then a cannonball lane, then the crossfire row.
    //-----------------------------------------------------------

    const auto laneAt =
        [](const std::vector<const Lane*>& rows, std::size_t index) -> const Lane*
        {
            return index < rows.size() ? rows[index] : nullptr;
        };

    const auto arrowRuns = FindRunsOfType(*level, LaneType::Arrow);
    const auto spikeRuns = FindRunsOfType(*level, LaneType::SpikeMud);
    const auto cannonRuns = FindRunsOfType(*level, LaneType::Cannonball);
    const auto grassRuns = FindRunsOfType(*level, LaneType::SafeGrass);

    const auto stash =
        [this](PieceType type, const Lane* target, float x)
        {
            if (!target)
                return;

            collectibleManager->Spawn(
                type,
                glm::vec3(x, target->GetSurfaceHeight(), target->GetCenterZ()));
        };

    // Bishop: the spear row, sandwiched between the two arrow lanes of the
    // first field. Reaching it means crossing one sweeping arrow, standing
    // in the lane the spears are thrown along, and crossing the second to
    // get out. Set well left of where those spears come down.
    if (arrowRuns.size() >= 1)
        stash(PieceType::Bishop, laneAt(arrowRuns[0], 1), -2.80f);

    // Knight: the middle of the first spike field, in the strip between the
    // mud on one side and the spike bed on the other, with the field's slow
    // arrow sweeping straight through it. The gap is wide enough to stand
    // in safely - the timing of the arrow is the whole test.
    if (spikeRuns.size() >= 1)
        stash(PieceType::Knight, laneAt(spikeRuns[0], 1), 0.75f);

    // Rook: inside the cannonball lane, on the far side from the rolling
    // log. Cannonballs cross this exact spot, so it has to be taken between
    // two of them; the shield is a fitting reward for standing in front of
    // one.
    if (cannonRuns.size() >= 1)
        stash(PieceType::Rook, laneAt(cannonRuns[0], 1), -2.90f);

    // Queen: the crossfire row, the hardest ground before the finale. Two
    // spears come down on this row from opposite sides, and she sits in the
    // band between their two impact zones - close enough to both that it
    // has to be timed, far enough from either to be safe once timed.
    if (grassRuns.size() >= 8)
        stash(PieceType::Queen, laneAt(grassRuns[7], 0), 0.75f);

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

void Game::LoadHudSprites()
{
    // Health pip -- a heart. Tinted red when filled, dim grey when empty.
    ResourceManager::LoadTexture(
        "hud_pip_square",
        "Src/Assets/Sprites/UI/hud_pip_square.png");

    // Checkpoint pip -- its own marker, distinct from the health heart.
    // Tinted per use, same as the health pip.
    ResourceManager::LoadTexture(
        "hud_checkpoint_pip",
        "Src/Assets/Sprites/UI/hud_checkpoint_pip.png");

    // Ability duration bar background/fill -- a plain square. A heart (or
    // any non-rectangular icon) stretched into a thin bar reads as broken,
    // so this stays its own flat placeholder rather than reusing an icon.
    ResourceManager::LoadTexture(
        "hud_bar_fill",
        "Src/Assets/Sprites/UI/hud_bar_fill.png");

    // Ability indicator: one universal icon regardless of which piece
    // granted the ability. The current-piece icon already shows which
    // piece is involved, so this slot just has to read as "ability
    // ready/active" at a glance.
    ResourceManager::LoadTexture(
        "hud_icon_ability",
        "Src/Assets/Sprites/UI/hud_icon_ability.png");

    // Interact prompt, shown near the pawn while it's in range of a gate
    // or the King's Cage.
    ResourceManager::LoadTexture(
        "hud_interact_prompt",
        "Src/Assets/Sprites/UI/hud_interact_prompt.png");

    // Game Over banner frame, shown briefly after the 3rd death. Tinted at
    // draw time (GameConfig::HudGameOverBannerTint) rather than baked red,
    // so the source art stays a neutral, reusable frame.
    ResourceManager::LoadTexture(
        "hud_game_over",
        "Src/Assets/Sprites/UI/hud_game_over.png");

    // "GAME OVER" text, drawn on top of the frame above. Pre-rendered with
    // its own baked color (gold fill, dark stroke), not tinted, so it stays
    // legible regardless of the frame's tint.
    ResourceManager::LoadTexture(
        "hud_game_over_text",
        "Src/Assets/Sprites/UI/hud_game_over_text.png");

    // Victory banner frame + text, shown once when the pawn reaches the
    // King. Same construction as the Game Over banner, different art/tint
    // so the two never read as the same event.
    ResourceManager::LoadTexture(
        "hud_victory",
        "Src/Assets/Sprites/UI/hud_victory.png");

    ResourceManager::LoadTexture(
        "hud_victory_text",
        "Src/Assets/Sprites/UI/hud_victory_text.png");

    // Controls screen keycaps (WASD + Space). E reuses hud_interact_prompt
    // above rather than a separate texture.
    ResourceManager::LoadTexture(
        "hud_key_w", "Src/Assets/Sprites/UI/keyboard_w.png");
    ResourceManager::LoadTexture(
        "hud_key_a", "Src/Assets/Sprites/UI/keyboard_a.png");
    ResourceManager::LoadTexture(
        "hud_key_s", "Src/Assets/Sprites/UI/keyboard_s.png");
    ResourceManager::LoadTexture(
        "hud_key_d", "Src/Assets/Sprites/UI/keyboard_d.png");
    ResourceManager::LoadTexture(
        "hud_key_space", "Src/Assets/Sprites/UI/keyboard_space.png");

    // One silhouette per PieceType, used by the current-piece indicator.
    for (int i = 0; i < PieceTypeCount; ++i)
    {
        const PieceType type = PieceTypeFromIndex(i);

        ResourceManager::LoadTexture(
            PieceIconTextureName(type),
            std::string("Src/Assets/Sprites/UI/") +
                PieceIconTextureName(type) + ".png");
    }
}

const std::string& Game::PieceIconTextureName(PieceType type)
{
    static const std::unordered_map<PieceType, std::string> names = {
        { PieceType::Pawn,        "hud_icon_Pawn" },
        { PieceType::Rook,        "hud_icon_Rook" },
        { PieceType::Knight,      "hud_icon_Knight" },
        { PieceType::Bishop,      "hud_icon_Bishop" },
        { PieceType::Queen,       "hud_icon_Queen" },
        { PieceType::King,        "hud_icon_King" },
        { PieceType::MountedPawn, "hud_icon_MountedPawn" },
    };

    return names.at(type);
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

void Game::AppendSpearSprites(std::vector<Sprite>& frameSprites) const
{
    if (!hazardManager)
        return;

    // Amber, so it belongs to neither of the two colours already spoken for:
    // red is burning ground and blue is an incoming bolt. A spear is a
    // physical thing landing, and reads as its own threat.
    const glm::vec3 spearTint(1.0f, 0.72f, 0.22f);

    for (const auto& hazard : hazardManager->GetHazards())
    {
        if (!hazard)
            continue;

        const Hazard* hazardVisual =
            dynamic_cast<const Hazard*>(&hazard->GetVisual());

        if (!hazardVisual)
            continue;

        const HazardType type = hazardVisual->GetType();

        // In the air: mark where it is going to come down.
        //
        // The spear itself is a mesh and the 3D pass draws it following the
        // arc, so this is purely the fairness marker - without it the player
        // has to judge a parabola's landing point by eye, which is not a
        // dodge so much as a guess.
        if (type == HazardType::Spear &&
            hazard->GetMovementPattern() == HazardMovementPattern::ArcProjectile)
        {
            const float duration =
                std::max(hazard->GetArcDuration(), 0.001f);

            const float t = std::clamp(
                hazard->GetArcElapsed() / duration,
                0.0f,
                1.0f);

            const glm::vec3 landing = hazard->GetArcLandingPosition();

            Sprite marker = Sprite::CreateGroundDecal(
                "hazard_circle",
                landing,
                glm::vec2(
                    GameConfig::SpearImpactRadius * 2.0f,
                    GameConfig::SpearImpactRadius * 2.0f));

            marker.tint = spearTint;

            // Tightens as the spear falls, so the marker is a countdown
            // rather than a static decoration.
            marker.opacity = GameConfig::SpearTelegraphOpacity * (0.35f + 0.65f * t);
            marker.layer = 13;

            frameSprites.push_back(marker);
            continue;
        }

        // Landed: the broken ground, sized from the radius the collision
        // pass actually tests so the ring is the danger rather than a hint
        // at it.
        if (type == HazardType::SpearImpact)
        {
            const float duration =
                std::max(hazard->GetZoneDuration(), 0.001f);

            const float t = std::clamp(
                hazard->GetZoneElapsed() / duration,
                0.0f,
                1.0f);

            Sprite ring = Sprite::CreateGroundDecal(
                "hazard_circle",
                hazard->GetVisual().GetGroundPosition(),
                glm::vec2(
                    GameConfig::SpearImpactRadius * 2.0f,
                    GameConfig::SpearImpactRadius * 2.0f));

            ring.tint = spearTint;

            // Fades out as the danger does, so it never outlives what it is
            // warning about.
            ring.opacity = GameConfig::SpearImpactRingOpacity * (1.0f - t);
            ring.layer = 15;

            frameSprites.push_back(ring);
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

void Game::AppendHudSprites(std::vector<Sprite>& frameSprites) const
{
    if (!pawn)
        return;

    constexpr int LayerBack = 200;
    constexpr int LayerFill = 201;
    constexpr int LayerIcon = 202;

    // ---- Health pips (top-left) ----
    {
        const int maxPips = static_cast<int>(GameConfig::MaxPawnHealth);
        const int filledPips = static_cast<int>(std::round(pawn->GetHealth()));

        for (int i = 0; i < maxPips; ++i)
        {
            Sprite pip = Sprite::CreateScreen(
                "hud_pip_square",
                glm::vec2(
                    GameConfig::HudMargin +
                        GameConfig::HudHealthPipSize * 0.5f +
                        i * (GameConfig::HudHealthPipSize +
                            GameConfig::HudHealthPipSpacing),
                    GameConfig::HudMargin +
                        GameConfig::HudHealthPipSize * 0.5f),
                glm::vec2(
                    GameConfig::HudHealthPipSize,
                    GameConfig::HudHealthPipSize));

            pip.tint = (i < filledPips)
                ? GameConfig::HudHealthFilledTint
                : GameConfig::HudEmptyTint;
            pip.layer = LayerFill;

            frameSprites.push_back(pip);
        }
    }

    // ---- Checkpoint pips (top-centre) ----
    {
        const int pipCount = static_cast<int>(checkpointGates.size());
        const float totalWidth =
            pipCount * GameConfig::HudCheckpointPipSize +
            std::max(0, pipCount - 1) * GameConfig::HudCheckpointPipSpacing;
        const float startX = hudScreenWidth * 0.5f - totalWidth * 0.5f;

        for (int i = 0; i < pipCount; ++i)
        {
            Sprite pip = Sprite::CreateScreen(
                "hud_checkpoint_pip",
                glm::vec2(
                    startX +
                        GameConfig::HudCheckpointPipSize * 0.5f +
                        i * (GameConfig::HudCheckpointPipSize +
                            GameConfig::HudCheckpointPipSpacing),
                    GameConfig::HudMargin +
                        GameConfig::HudCheckpointPipSize * 0.5f),
                glm::vec2(
                    GameConfig::HudCheckpointPipSize,
                    GameConfig::HudCheckpointPipSize));

            const bool activated =
                i < static_cast<int>(checkpointGateActivated.size()) &&
                checkpointGateActivated[i];

            pip.tint = activated
                ? GameConfig::HudFilledTint
                : GameConfig::HudEmptyTint;
            pip.layer = LayerFill;

            frameSprites.push_back(pip);
        }
    }

    // ---- Ability icon + duration bar (top-right) ----
    {
        const glm::vec2 iconCenter(
            hudScreenWidth - GameConfig::HudMargin -
                GameConfig::HudAbilityIconSize * 0.5f,
            GameConfig::HudMargin +
                GameConfig::HudAbilityIconSize * 0.5f);

        // One universal icon whenever an ability is banked or active --
        // which piece granted it is already shown by the current-piece
        // icon below, so this slot just needs to read as "ability ready".
        const bool haveAbility =
            pawn->IsAbilityActive() || pawn->HasStoredPiece();

        if (haveAbility)
        {
            Sprite icon = Sprite::CreateScreen(
                "hud_icon_ability",
                iconCenter,
                glm::vec2(
                    GameConfig::HudAbilityIconSize,
                    GameConfig::HudAbilityIconSize));

            icon.layer = LayerIcon;
            frameSprites.push_back(icon);
        }

        // Duration bar only while a timed ability is actually running --
        // GetAbilityDurationFraction() is 0 for Bishop, whose effect is
        // instant, so its icon appears with no bar under it.
        const float fraction = pawn->GetAbilityDurationFraction();

        if (fraction > 0.0f)
        {
            const glm::vec2 barCenter(
                iconCenter.x,
                iconCenter.y +
                    GameConfig::HudAbilityIconSize * 0.5f +
                    GameConfig::HudAbilityBarGap +
                    GameConfig::HudAbilityBarHeight * 0.5f);

            Sprite back = Sprite::CreateScreen(
                "hud_bar_fill",
                barCenter,
                glm::vec2(
                    GameConfig::HudAbilityBarWidth,
                    GameConfig::HudAbilityBarHeight));

            back.tint = GameConfig::HudBarBackTint;
            back.layer = LayerBack;
            frameSprites.push_back(back);

            // Left-anchored fill: shrink the quad and re-centre it so it
            // drains from right to left as the ability runs out, rather
            // than shrinking from the centre outward.
            const float fillWidth = GameConfig::HudAbilityBarWidth * fraction;

            Sprite fill = Sprite::CreateScreen(
                "hud_bar_fill",
                glm::vec2(
                    barCenter.x -
                        GameConfig::HudAbilityBarWidth * 0.5f +
                        fillWidth * 0.5f,
                    barCenter.y),
                glm::vec2(fillWidth, GameConfig::HudAbilityBarHeight));

            fill.tint = GameConfig::HudFilledTint;
            fill.layer = LayerFill;
            frameSprites.push_back(fill);
        }
    }

    // ---- Current piece icon (bottom-centre) ----
    {
        Sprite icon = Sprite::CreateScreen(
            PieceIconTextureName(pawn->GetCharacter()),
            glm::vec2(
                hudScreenWidth * 0.5f,
                hudScreenHeight - GameConfig::HudMargin -
                    GameConfig::HudPieceIconSize * 0.5f),
            glm::vec2(
                GameConfig::HudPieceIconSize,
                GameConfig::HudPieceIconSize));

        icon.layer = LayerIcon;
        frameSprites.push_back(icon);
    }

    // ---- Interact prompt (centred, above the piece icon) ----
    if (IsNearInteractable(pawn->GetTransform().GetPosition()))
    {
        Sprite prompt = Sprite::CreateScreen(
            "hud_interact_prompt",
            glm::vec2(
                hudScreenWidth * 0.5f,
                hudScreenHeight - GameConfig::HudMargin -
                    GameConfig::HudPieceIconSize -
                    GameConfig::HudInteractPromptGap -
                    GameConfig::HudInteractPromptSize * 0.5f),
            glm::vec2(
                GameConfig::HudInteractPromptSize,
                GameConfig::HudInteractPromptSize));

        prompt.layer = LayerIcon;
        frameSprites.push_back(prompt);
    }

    // ---- Game Over banner (screen-centred, on top of everything) ----
    if (gameOverBannerTimer > 0.0f)
    {
        constexpr int LayerGameOverOverlay = 210;
        constexpr int LayerGameOverBanner = 211;
        constexpr int LayerGameOverText = 212;

        // Dim the world behind the banner so it reads clearly against
        // whatever's on screen underneath.
        Sprite overlay = Sprite::CreateScreen(
            "hud_bar_fill",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(hudScreenWidth, hudScreenHeight));

        overlay.tint = GameConfig::HudGameOverOverlayTint;
        overlay.opacity = GameConfig::HudGameOverOverlayOpacity;
        overlay.layer = LayerGameOverOverlay;
        frameSprites.push_back(overlay);

        Sprite banner = Sprite::CreateScreen(
            "hud_game_over",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(
                GameConfig::HudGameOverBannerSize,
                GameConfig::HudGameOverBannerSize));

        banner.tint = GameConfig::HudGameOverBannerTint;
        banner.layer = LayerGameOverBanner;
        frameSprites.push_back(banner);

        Sprite text = Sprite::CreateScreen(
            "hud_game_over_text",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(
                GameConfig::HudGameOverTextWidth,
                GameConfig::HudGameOverTextHeight));

        text.layer = LayerGameOverText;
        frameSprites.push_back(text);
    }

    // ---- Victory banner (screen-centred, on top of everything) ----
    if (victoryBannerTimer > 0.0f)
    {
        constexpr int LayerVictoryOverlay = 220;
        constexpr int LayerVictoryBanner = 221;
        constexpr int LayerVictoryText = 222;

        Sprite overlay = Sprite::CreateScreen(
            "hud_bar_fill",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(hudScreenWidth, hudScreenHeight));

        overlay.tint = GameConfig::HudGameOverOverlayTint;
        overlay.opacity = GameConfig::HudGameOverOverlayOpacity;
        overlay.layer = LayerVictoryOverlay;
        frameSprites.push_back(overlay);

        Sprite banner = Sprite::CreateScreen(
            "hud_victory",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(
                GameConfig::HudVictoryBannerSize,
                GameConfig::HudVictoryBannerSize));

        banner.tint = GameConfig::HudVictoryBannerTint;
        banner.layer = LayerVictoryBanner;
        frameSprites.push_back(banner);

        Sprite text = Sprite::CreateScreen(
            "hud_victory_text",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(
                GameConfig::HudVictoryTextWidth,
                GameConfig::HudVictoryTextHeight));

        text.layer = LayerVictoryText;
        frameSprites.push_back(text);
    }

    // ---- Controls screen (screen-centred, first few seconds of a run) ----
    if (controlsScreenTimer > 0.0f)
    {
        constexpr int LayerControlsOverlay = 230;
        constexpr int LayerControlsKeys = 231;

        Sprite overlay = Sprite::CreateScreen(
            "hud_bar_fill",
            glm::vec2(hudScreenWidth * 0.5f, hudScreenHeight * 0.5f),
            glm::vec2(hudScreenWidth, hudScreenHeight));

        overlay.tint = GameConfig::HudControlsOverlayTint;
        overlay.opacity = GameConfig::HudControlsOverlayOpacity;
        overlay.layer = LayerControlsOverlay;
        frameSprites.push_back(overlay);

        const float key = GameConfig::HudControlsKeySize;
        const float gap = GameConfig::HudControlsKeyGap;
        const float groupGap = GameConfig::HudControlsGroupGap;

        // WASD diamond: S is the reference point, A/D flank it, W sits
        // above. Space and E sit to the right, vertically centred on the
        // A/S/D row.
        const float totalWidth =
            (key + gap) * 3 + groupGap + key + groupGap + key;

        const float clusterLeft = hudScreenWidth * 0.5f - totalWidth * 0.5f;
        const float rowY = hudScreenHeight * 0.5f;

        const glm::vec2 sPos(clusterLeft + key + gap + key * 0.5f, rowY);
        const glm::vec2 aPos(sPos.x - (key + gap), rowY);
        const glm::vec2 dPos(sPos.x + (key + gap), rowY);
        const glm::vec2 wPos(sPos.x, rowY - (key + gap));

        const glm::vec2 keySize(key, key);

        Sprite wKey = Sprite::CreateScreen("hud_key_w", wPos, keySize);
        wKey.layer = LayerControlsKeys;
        frameSprites.push_back(wKey);

        Sprite aKey = Sprite::CreateScreen("hud_key_a", aPos, keySize);
        aKey.layer = LayerControlsKeys;
        frameSprites.push_back(aKey);

        Sprite sKey = Sprite::CreateScreen("hud_key_s", sPos, keySize);
        sKey.layer = LayerControlsKeys;
        frameSprites.push_back(sKey);

        Sprite dKey = Sprite::CreateScreen("hud_key_d", dPos, keySize);
        dKey.layer = LayerControlsKeys;
        frameSprites.push_back(dKey);

        const float spaceX = dPos.x + key * 0.5f + groupGap + key * 0.5f;
        const glm::vec2 spacePos(spaceX, rowY);

        Sprite spaceKey = Sprite::CreateScreen(
            "hud_key_space", spacePos, keySize);
        spaceKey.layer = LayerControlsKeys;
        frameSprites.push_back(spaceKey);

        const float ePosX = spaceX + key * 0.5f + groupGap + key * 0.5f;
        const glm::vec2 ePos(ePosX, rowY);

        Sprite eKey = Sprite::CreateScreen(
            "hud_interact_prompt", ePos, keySize);
        eKey.layer = LayerControlsKeys;
        frameSprites.push_back(eKey);
    }
}

bool Game::IsNearInteractable(const glm::vec3& pawnPosition) const
{
    for (const auto& gate : checkpointGates)
    {
        if (!gate)
            continue;

        if (glm::length(pawnPosition - gate->GetGroundPosition()) <=
            GameConfig::InteractRadius)
        {
            return true;
        }
    }

    if (kingsCage &&
        glm::length(pawnPosition - kingsCage->GetGroundPosition()) <=
            GameConfig::InteractRadius)
    {
        return true;
    }

    return false;
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

        checkpointGateActivated[i] = true;

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

        if (newAngle >= CheckpointGate::GetMaxDoorAngle()) {
            checkpointGateOpening[i] = false;

            AudioManager::PlaySound("Src/Resources/Sounds/door-open.wav");
        }
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

            leftDoor->GetTransform().SetRotation(
                0.0f, -kingsCageDoorAngle, 0.0f);

            rightDoor->GetTransform().SetRotation(
                0.0f, kingsCageDoorAngle, 0.0f);

            if (kingsCageDoorAngle >= GameConfig::KingsCageMaxDoorAngle)
            {
                kingsCageDoorOpening = false;

                // Play sound when the King's Cage door finishes opening
                AudioManager::PlaySound("Src/Resources/Sounds/door-open.wav");
            }
        }
    }
}

void Game::UpdateGameOver(float deltaTime)
{
    if (gameOverBannerTimer > 0.0f)
        gameOverBannerTimer = std::max(0.0f, gameOverBannerTimer - deltaTime);

    if (!pawn || !pawn->ConsumeDeathPulse())
        return;

    ++deathCount;

    if (deathCount < GameConfig::MaxDeathsBeforeGameOver)
        return;

    AudioManager::PlaySound("Src/Resources/Sounds/defeat.wav");

    // Game Over: harsher than a normal death. Clear checkpoint progress and
    // send the pawn all the way back to the level's initial spawn point,
    // not whichever checkpoint it had most recently banked -- otherwise the
    // very next death would undo this reset immediately.
    deathCount = 0;
    gameOverBannerTimer = GameConfig::GameOverBannerDuration;

    std::fill(
        checkpointGateActivated.begin(),
        checkpointGateActivated.end(),
        false);

    if (level)
    {
        pawn->SetSpawnPosition(level->GetPlayerSpawnPosition());
        pawn->Respawn();
    }
}

void Game::UpdateVictory(float deltaTime)
{
    if (victoryBannerTimer > 0.0f)
    {
        victoryBannerTimer = std::max(0.0f, victoryBannerTimer - deltaTime);

        // Reset back to the start the instant the banner finishes -- same
        // scope as Game Over (checkpoints clear, deaths clear, pawn goes
        // back to the level's initial spawn) plus re-locking the cage,
        // since that's specific to what Victory itself requires. Clearing
        // hasWon makes the whole thing repeatable, exactly like Game Over.
        if (victoryBannerTimer <= 0.0f && hasWon)
        {
            hasWon = false;
            deathCount = 0;

            std::fill(
                checkpointGateActivated.begin(),
                checkpointGateActivated.end(),
                false);

            kingsCageDoorAngle = 0.0f;
            kingsCageDoorOpening = false;

            if (kingsCage)
            {
                if (WorldObject* leftDoor = kingsCage->GetLeftDoor())
                    leftDoor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);

                if (WorldObject* rightDoor = kingsCage->GetRightDoor())
                    rightDoor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
            }

            if (level && pawn)
            {
                pawn->SetSpawnPosition(level->GetPlayerSpawnPosition());
                pawn->Respawn();
            }
        }
    }

    if (hasWon || !pawn || !capturedKing)
        return;

    // The cage has to actually be opened first -- otherwise walking near a
    // still-locked cage would count as "getting the king".
    if (kingsCageDoorAngle < GameConfig::KingsCageMaxDoorAngle)
        return;

    const float distance = glm::length(
        pawn->GetTransform().GetPosition() - capturedKing->GetGroundPosition());

    if (distance > GameConfig::KingRescueRadius)
        return;

    hasWon = true;
    victoryBannerTimer = GameConfig::VictoryBannerDuration;
    AudioManager::PlaySound("Src/Resources/Sounds/victory.wav");
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
    // resolve it here against whatever's actually in the level: the king's
    // cage door, then the checkpoint gate, then finally the pawn's own
    // banked ability if neither was in range.
    //
    // Cage before checkpoint deliberately: the 3rd checkpoint sits only
    // 1.5 units from the cage (InteractRadius is 1.6), so the two ranges
    // overlap. Re-triggering an already-open checkpoint is a harmless
    // no-op; missing the cage because the checkpoint claimed the press
    // first made the win condition unreachable from most approach angles.
    if (pawn && pawn->ConsumeInteractPulse())
    {
        const glm::vec3 pawnPosition = pawn->GetTransform().GetPosition();

        const bool interactedWithWorld =
            TryInteractWithKingsCage(pawnPosition) ||
            TryInteractWithCheckpointGate(pawnPosition);

        if (!interactedWithWorld)
            pawn->TryActivateAbility();
    }

    UpdateDoors(deltaTime);

    // After collision resolution (where a death this frame would have
    // happened) and before the camera, so a Game Over's reset position is
    // what the camera follows this same frame rather than lagging a frame
    // behind.
    UpdateGameOver(deltaTime);
    UpdateVictory(deltaTime);

    if (controlsScreenTimer > 0.0f)
        controlsScreenTimer = std::max(0.0f, controlsScreenTimer - deltaTime);

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
    AppendSpearSprites(frameSprites);
    AppendAbilityPulseSprites(frameSprites);
    AppendHudSprites(frameSprites);

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
    hudScreenWidth = width;
    hudScreenHeight = height;

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

    AudioManager::StopMusic();
    AudioManager::Shutdown();
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

int Game::GetActivatedCheckpointCount() const
{
    return static_cast<int>(std::count(
        checkpointGateActivated.begin(),
        checkpointGateActivated.end(),
        true));
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