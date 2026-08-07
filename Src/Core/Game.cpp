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

    const int row = level->FindRowOfType(LaneType::Checkpoint);

    const Lane* lane = level->GetLane(row);

    if (!lane)
        return;

    checkpointGate = std::make_shared<CheckpointGate>();

    checkpointGate->Build(*gateMeshes);

    // Centred on the board and standing on the checkpoint lane's own surface.
    checkpointGate->SetGroundPosition(
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

    auto place = [this](ObstacleType type, float x, const Lane& lane)
    {
        auto obstacle = obstacleMeshes->CreateObstacle(type);

        obstacle->SetGroundPosition(
            glm::vec3(x, lane.GetSurfaceHeight(), lane.GetCenterZ()));

        stationaryHazards.push_back(obstacle);
    };

    // SpikeMud section (3 rows): Spikes and Mud alternate left/right down
    // the section, per the GDD's own "[S] [M]     [S]     [M] [S]" map, so
    // no two consecutive rows block the same corridor.
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::SpikeMud);

        if (rows.size() >= 1 && rows[0])
        {
            place(ObstacleType::Spikes, -halfWidth * 0.55f, *rows[0]);
            place(ObstacleType::Mud,     halfWidth * 0.24f, *rows[0]);
        }

        if (rows.size() >= 2 && rows[1])
        {
            place(ObstacleType::Mud,    -halfWidth * 0.24f, *rows[1]);
            place(ObstacleType::Spikes,  halfWidth * 0.55f, *rows[1]);
        }

        if (rows.size() >= 3 && rows[2])
        {
            place(ObstacleType::Spikes, -halfWidth * 0.70f, *rows[2]);
            place(ObstacleType::Mud,    -halfWidth * 0.35f, *rows[2]);
            // Right half of this row stays clear.
        }
    }

    // FenceTree section (3 rows): the GDD's top-down map names exactly
    // Tree/Fence/Rock/Wall for this section; Palisade and Bush fill the
    // third row since the obstacle set has more props than the map's
    // single simplified row shows.
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::FenceTree);

        if (rows.size() >= 1 && rows[0])
        {
            place(ObstacleType::Tree,  -halfWidth * 0.55f, *rows[0]);
            place(ObstacleType::Fence,  halfWidth * 0.45f, *rows[0]);
        }

        if (rows.size() >= 2 && rows[1])
        {
            place(ObstacleType::Rock, -halfWidth * 0.40f, *rows[1]);

            // Wall fully blocks and can neither be jumped nor broken (GDD),
            // so it sits near the edge and leaves the wide side as the
            // real route.
            place(ObstacleType::Wall,  halfWidth * 0.73f, *rows[1]);
        }

        if (rows.size() >= 3 && rows[2])
        {
            place(ObstacleType::Palisade, -halfWidth * 0.44f, *rows[2]);
            place(ObstacleType::Bush,      halfWidth * 0.22f, *rows[2]);
            // Right side of this row (roughly x = 1.5..4.5) stays clear --
            // BuildLevelCollectibles puts the Queen there.
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
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::Arrow);

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
                    // it was fired along. SpawnCurvedHazard is left in place
                    // for the fireball, which the GDD does want curving.
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
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::FenceTree);

        if (!rows.empty() && rows.front())
        {
            hazardManager->SpawnCow(
                glm::vec3(
                    0.0f,
                    rows.front()->GetSurfaceHeight(),
                    rows.front()->GetCenterZ()),
                2.5f);
        }
    }

    // FireballLightning section (3 rows): a repeating curved Fireball
    // sweep (burn patches are already automatic in HazardManager::Update),
    // two Lightning warning zones flanking a clear centre corridor, and
    // RollingLog rolling through the whole span, faster and with a
    // smaller hit area than the Rolling Rock gauntlet below, per the GDD.
    {
        auto rows = FindConsecutiveRowsOfType(*level, LaneType::FireballLightning);

        if (rows.size() >= 1 && rows[0])
        {
            const float y = rows[0]->GetSurfaceHeight();
            const float z = rows[0]->GetCenterZ();

            hazardManager->RegisterRepeatingSpawn(
                4.0f,
                [this, y, z, halfWidth]()
                {
                    hazardManager->SpawnCurvedHazard(
                        HazardType::Fireball,
                        glm::vec3(-halfWidth - 0.5f, y, z),
                        glm::vec3(halfWidth + 0.5f, y, z),
                        3.4f,
                        1.3f);
                });
        }

        if (rows.size() >= 2 && rows[1])
        {
            const float y = rows[1]->GetSurfaceHeight();
            const float z = rows[1]->GetCenterZ();

            // "An unavoidable strike if the player stays in a marked
            // danger area for too long." Warning phase gives a chance to
            // move away; centre corridor between the two zones stays clear.
            hazardManager->SpawnWarningHazard(
                glm::vec3(-halfWidth * 0.55f, y, z), 1.5f, 1.0f);

            hazardManager->SpawnWarningHazard(
                glm::vec3(halfWidth * 0.55f, y, z), 1.5f, 1.0f);
        }

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
    }

    // Rolling Rock gauntlet: three boulders roll the length of the field
    // from just outside the King's Cage back toward the Checkpoint,
    // reading as the battlefield collapsing in behind the player rather
    // than a short preview confined to one section.
    //
    // Playtest feedback: a single rock rolling only through the 3-row
    // Cannonball section didn't read as a real threat. Moving the start
    // to the King's Cage and adding two more lanes gives it the length
    // and coverage to feel like one.
    {
        auto kingsCageRows = FindConsecutiveRowsOfType(*level, LaneType::KingsCage);
        const int checkpointRow = level->FindRowOfType(LaneType::Checkpoint);

        const Lane* startLane = kingsCageRows.empty() ? nullptr : kingsCageRows.back();
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

    // Lightning: timed warning marker, then a brief damaging strike. The
    // hazard itself is a transform-only anchor; AppendLightningSprites draws
    // its warning, bolt and impact frames from the sprite pack.
    {
        const int row = level->FindRowOfType(LaneType::FireballLightning);

        if (const Lane* lane = level->GetLane(row))
        {
            hazardManager->SpawnWarningHazard(
                glm::vec3(
                    0.0f,
                    lane->GetSurfaceHeight(),
                    lane->GetCenterZ()),
                0.50f,
                1.0f);
        }
    }

    // Fireball: curved projectile that leaves a temporary impact zone when
    // it reaches its destination. HazardManager handles the lingering zone;
    // AppendFireballSprites draws both phases from the sprite pack.
    {
        const int row = level->FindRowOfType(LaneType::FireballLightning);

        if (const Lane* lane = level->GetLane(row))
        {
            const float y = lane->GetSurfaceHeight();
            const float z = lane->GetCenterZ() - 1.0f;
            const float range = halfWidth * 0.65f;

            hazardManager->RegisterRepeatingSpawn(
                4.5f,
                [this, y, z, range]()
                {
                    hazardManager->SpawnCurvedHazard(
                        HazardType::Fireball,
                        glm::vec3(-range, y, z),
                        glm::vec3(range, y, z),
                        3.2f,
                        1.0f);
                });

            hazardManager->RegisterRepeatingSpawn(
                5.0f,
                [this, y, z, range]()
                {
                    hazardManager->SpawnCurvedHazard(
                        HazardType::Fireball,
                        glm::vec3(range, y, z - 1.0f),
                        glm::vec3(-range, y, z - 1.0f),
                        3.2f,
                        -1.0f);
                });
        }
    }
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

        if (!hazard->IsActive())
        {
            const float warningDuration =
                std::max(hazard->GetWarningDuration(), 0.001f);
            const float warningT = std::clamp(
                hazard->GetPhaseElapsed() / warningDuration,
                0.0f,
                1.0f);

            Sprite warning = Sprite::CreateGroundDecal(
                "lightning_warning_circle",
                ground,
                glm::vec2(2.8f, 2.8f));

            warning.tint = glm::vec3(1.0f, 0.0f, 0.0f);
            warning.opacity = 0.16f + 0.10f * warningT;
            warning.layer = 10;

            frameSprites.push_back(warning);
            continue;
        }

        const float strikeDuration =
            std::max(hazard->GetStrikeDuration(), 0.001f);
        const float t = std::clamp(
            hazard->GetPhaseElapsed() / strikeDuration,
            0.0f,
            1.0f);

        if (t < 0.72f)
        {
            const int frame = std::clamp(
                1 + static_cast<int>((t / 0.72f) * 5.0f),
                1,
                5);

            Sprite bolt = Sprite::CreateBillboard(
                NumberedTextureName("lightning_beginning", frame),
                ground + glm::vec3(0.0f, 1.45f, 0.0f),
                glm::vec2(1.0f, 3.0f));

            bolt.layer = 25;
            frameSprites.push_back(bolt);
        }
        else if (t < 0.9f)
        {
            const float endT = (t - 0.72f) / 0.18f;
            const int frame = std::clamp(
                1 + static_cast<int>(endT * 3.0f),
                1,
                3);

            Sprite bolt = Sprite::CreateBillboard(
                NumberedTextureName("lightning_end", frame),
                ground + glm::vec3(0.0f, 1.45f, 0.0f),
                glm::vec2(1.0f, 3.0f));

            bolt.layer = 25;
            frameSprites.push_back(bolt);
        }

        const int explosionFrame = std::clamp(
            1 + static_cast<int>(((std::max(t, 0.9f) - 0.9f) / 0.8f) * 10.0f),
            1,
            10);

        Sprite explosion = Sprite::CreateGroundDecal(
            NumberedTextureName("lightning_explosion", explosionFrame),
            ground,
            glm::vec2(12.0f, 12.0f));

        explosion.opacity = 0.9f;
        explosion.layer = 20;

        if (t >= 0.9f)
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

        if (!hazardVisual ||
            hazardVisual->GetType() != HazardType::Fireball)
        {
            continue;
        }

        const glm::vec3 ground = hazard->GetVisual().GetGroundPosition();

        if (hazard->GetMovementPattern() ==
            HazardMovementPattern::CurvedSweep)
        {
            const float duration =
                std::max(hazard->GetCurveDuration(), 0.001f);
            const float t = std::clamp(
                hazard->GetCurveElapsed() / duration,
                0.0f,
                1.0f);
            const int frame = std::clamp(
                1 + static_cast<int>(t * 8.0f),
                1,
                8);

            Sprite fireball = Sprite::CreateBillboard(
                NumberedTextureName("fire_spell", frame),
                ground + glm::vec3(0.0f, 0.75f, 0.0f),
                glm::vec2(1.6f, 0.9f));

            fireball.flipX = hazard->GetVelocity().x > 0.0f;
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
            const int frame = std::clamp(
                1 + static_cast<int>(t * 10.0f),
                1,
                10);

            Sprite impact = Sprite::CreateGroundDecal(
                NumberedTextureName("fireball_explosion", frame),
                ground,
                glm::vec2(3.5f, 3.5f));

            impact.opacity = 0.95f;
            impact.layer = 18;

            frameSprites.push_back(impact);
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
    if (!checkpointGate)
        return false;

    const float distance = glm::length(
        pawnPosition - checkpointGate->GetGroundPosition());

    if (distance > GameConfig::InteractRadius)
        return false;

    if (checkpointGate->GetDoorAngle() < CheckpointGate::GetMaxDoorAngle())
        checkpointGateOpening = true;

    // Standing at the gate at all means E belongs to it, not the ability --
    // even if it's already fully open and there's nothing left to start.
    return true;
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
    if (checkpointGateOpening && checkpointGate)
    {
        const float newAngle = std::min(
            checkpointGate->GetDoorAngle() +
                GameConfig::DoorOpenSpeed * deltaTime,
            CheckpointGate::GetMaxDoorAngle());

        checkpointGate->SetDoorAngle(newAngle);

        if (newAngle >= CheckpointGate::GetMaxDoorAngle())
            checkpointGateOpening = false;
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

        // The checkpoint gate is a structure, not a hazard: its walls and
        // leaves only need to be solid. Rebuilt from the gate every frame so
        // the leaves' boxes follow the swing, which is what keeps the
        // collision and the opening animation in step.
        if (checkpointGate)
        {
            gateCollisionBoxes.clear();
            checkpointGate->AppendCollisionBoxes(gateCollisionBoxes);
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

    // Bishop's ability mutates the world (removing nearby hazards)
    if (pawn && pawn->ConsumeBishopActivationPulse() && hazardManager)
    {
        hazardManager->RemoveNearest(
            pawn->GetTransform().GetPosition(),
            GameConfig::BishopRemovalCount);
    }

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

    if (checkpointGate)
        renderer->Draw(checkpointGate->GetShadow());

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

    // The gate is several meshes rather than one, because its two leaves
    // turn on their own hinges.
    if (checkpointGate)
    {
        for (const auto& part : checkpointGate->GetParts())
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
    checkpointGate.reset();

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
    return checkpointGate.get();
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
