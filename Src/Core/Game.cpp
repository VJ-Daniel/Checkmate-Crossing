/*
    ============================================================
    Checkmate Crossing - Game Orchestration Module

    Coordinates the camera, world renderer, battlefield and pawn, and
    owns the order in which they are created, updated and drawn.

    Based on the Gangster Survival OpenGL framework by Leonardo Moura.
    ============================================================
*/

#include "Game.h"

#include "GameConfig.h"
#include "ResourceManager.h"
#include "Shader.h"
#include <iostream>  // For debug output

namespace
{
    // TEMPORARY showcase settings. See Game::BuildShowcase.
    //
    // Three rows, two lanes apart, ordered by height: the tall rows have to
    // sit nearer the camera because the far end of the view runs out of
    // vertical room, and a tall row would be cut off at the top.
    //
    // That ordering also decides the occlusion. A prop of height h hides the
    // ground for about h / tan(pitch) behind it, which at 35 degrees is
    // roughly 1.4 h - so a 1.55 tall tree blots out the next two lanes. The
    // stationary row survives this only because its two tall entries, the
    // tree and the palisade, sit at the far ends of the row where no hazard
    // is placed behind them.

    constexpr int PieceRowLanesAhead = 2;
    constexpr int ObstacleRowLanesAhead = 4;
    constexpr int HazardRowLanesAhead = 6;

    /// Seven pieces now that the mounted pawn joined them, two of which are
    /// horses reaching about 0.58 forward from their slot.
    ///
    /// 1.2 puts the outer slots at 3.6, so the mounted pawn's nose stops
    /// around 4.18. At 1.35 it reached 4.63 and buried itself in the nearest
    /// border decoration, whose inner edge can fall as low as 4.33.
    constexpr float PieceSpacing = 1.2f;

    /// Eight props is the widest row. A spacing of 1.0 keeps the tree and
    /// palisade at about +/-3.5, with their outer edges near +/-4.0 and clear
    /// of the border-decoration band that begins at x = 4.8.
    constexpr float ObstacleSpacing = 1.0f;

    constexpr float HazardSpacing = 1.6f;
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

    hazardCollision->onBlocked = [this](const glm::vec3& position) {
        pawn->GetTransform().SetPosition(position);
        // Stop velocity when blocked
        pawn->SetVelocity(glm::vec3(0.0f));
        };

    hazardCollision->onSlowApplied = [this](float duration, float amount, bool linger) {
        if (linger == false && duration == 0.0f) {
            duration = 0.5f; // Minimum time so the timer registers!
        }
        // --------------------------

        pawn->ApplySlow(duration, amount);
        };

    // ---- Create Stationary Hazards ----
    CreateStationaryHazards();

    BuildCheckpointGate();
    BuildKingsCage();

    hazardManager = std::make_shared<HazardManager>(*obstacleMeshes);
    collectibleManager = std::make_shared<CollectibleManager>(*pieceMeshes);

    // ---------------------------------------------------------
    // REMOVED: BuildShowcase(); 
    // We don't want the pre-made showcase obstacles anymore.
    // ---------------------------------------------------------

    SpawnExampleHazards();
    SpawnExampleCollectibles();

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

glm::vec3 Game::GetShowcaseSlot(
    int lanesAhead,
    int slot,
    int slotCount,
    float spacing) const
{
    const int row = level->GetSpawnRow() + lanesAhead;

    // Stand the row on a real lane, so it rests on that lane's surface
    // rather than on an assumed height.
    const Lane* lane = level->GetLane(row);

    const float surface = lane
        ? lane->GetSurfaceHeight()
        : GameConfig::GroundSurface;

    const float firstX =
        -spacing * static_cast<float>(slotCount - 1) * 0.5f;

    return glm::vec3(
        firstX + spacing * static_cast<float>(slot),
        surface,
        Level::RowToWorldZ(row));
}

void Game::BuildShowcase()
{
    // ---------------------------------------------------------
    // REMOVED: Entirely disabled.
    // ---------------------------------------------------------
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

void Game::CreateStationaryHazards()
{
    if (!level || !obstacleMeshes)
        return;

    stationaryHazards.clear();

    const float halfWidth = Level::GetPlayableHalfWidth();
    const int spawnRow = level->GetSpawnRow();

    // Helper lambda to safely add stationary obstacles
    auto AddObstacle = [&](ObstacleType type, int rowOffset, float xOffset, float zOffset = 0.0f) {
        int row = spawnRow + rowOffset;
        if (const Lane* lane = level->GetLane(row))
        {
            const float y = lane->GetSurfaceHeight();
            const float z = lane->GetCenterZ();

            auto obstacle = obstacleMeshes->CreateObstacle(type);
            obstacle->SetGroundPosition(glm::vec3(xOffset, y, z + zOffset));
            obstacle->Initialize();
            stationaryHazards.push_back(obstacle);
        }
        };

    // ==========================================
    // LEFT LANE (x = -1.0) - Damage & Slow Testing
    // ==========================================

    // 1. SPIKES: Damage + Knockback (2 lanes ahead)
    AddObstacle(ObstacleType::Spikes, 2, -1.0f, 0.0f);

    // 2. MUD: Slows while inside + Lingers 2 seconds (3 lanes ahead)
    AddObstacle(ObstacleType::Mud, 3, -1.0f, 0.0f);

    // 3. BUSHES: Slows only while inside (4 lanes ahead)
    AddObstacle(ObstacleType::Bush, 4, -1.0f, 0.0f);

    // ==========================================
    // RIGHT LANE (x = 1.0) - Jumping Testing
    // ==========================================

    // 4. ROCK: Block movement, can be jumped over (5 lanes ahead)
    AddObstacle(ObstacleType::Rock, 5, 1.0f, 0.0f);

    // 5. FENCE: Block, Jumpable, Breakable (6 lanes ahead)
    AddObstacle(ObstacleType::Fence, 6, 1.0f, 0.0f);

    // 6. PALISADE: Jump over = Damage (7 lanes ahead)
    AddObstacle(ObstacleType::Palisade, 7, 1.0f, 0.0f);

    // 7. WALL: Completely Block, Unbreakable (8 lanes ahead)
    AddObstacle(ObstacleType::Wall, 8, 1.0f, 0.0f);

    // 8. TREE: Completely Block (9 lanes ahead)
    AddObstacle(ObstacleType::Tree, 9, 1.0f, 0.0f);

    std::cout << "Created " << stationaryHazards.size() << " stationary hazards for testing." << std::endl;
}

void Game::SpawnExampleHazards()
{
    if (!level || !hazardManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // Arrow lane: sweeps the full width and loops back forever, per the
    // GDD's "arrows... range covers the full horizontal width of the
    // map." It loops on its own, so it needs no repeating spawner.
    {
        const int row = level->GetSpawnRow() + 3;

        if (const Lane* lane = level->GetLane(row))
        {
            const float z = lane->GetCenterZ();
            const float y = lane->GetSurfaceHeight();

            hazardManager->SpawnLinearHazard(
                HazardType::Arrow,
                glm::vec3(-halfWidth - 0.5f, y, z),
                glm::vec3(halfWidth + 0.5f, y, z),
                3.0f,
                true);
        }
    }

    // Spear: "similar speed to arrows, but curved trajectory." Each flight
    // is one-shot, so a repeating spawner keeps a new one coming.
    {
        const int row = level->GetSpawnRow() + 5;

        if (const Lane* lane = level->GetLane(row))
        {
            const float z = lane->GetCenterZ();
            const float y = lane->GetSurfaceHeight();

            hazardManager->RegisterRepeatingSpawn(
                3.5f,
                [this, y, z, halfWidth]()
                {
                    hazardManager->SpawnCurvedHazard(
                        HazardType::Spear,
                        glm::vec3(-halfWidth - 0.5f, y, z),
                        glm::vec3(halfWidth + 0.5f, y, z),
                        3.2f,
                        1.2f);
                });
        }
    }

    // Cannonball: "faster than arrows... range is shorter and reaches
    // about 70% of the map's width." One-shot per launch, so it also
    // needs a repeating spawner.
    {
        const int row = level->GetSpawnRow() + 12;

        if (const Lane* lane = level->GetLane(row))
        {
            const float z = lane->GetCenterZ();
            const float y = lane->GetSurfaceHeight();
            const float range = halfWidth * 0.7f;

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

    // Rolling Rock / Rolling Log: the GDD calls both "vertical"
    // projectiles, meaning they roll along a lane's depth (Z), not
    // sideways across it like arrows/cannonballs.
    {
        const int startRow = level->GetSpawnRow() + 7;
        const int endRow = level->GetSpawnRow() + 9;

        if (const Lane* startLane = level->GetLane(startRow))
        {
            const float y = startLane->GetSurfaceHeight();
            const float startZ = Level::RowToWorldZ(startRow);
            const float endZ = Level::RowToWorldZ(endRow);

            hazardManager->RegisterRepeatingSpawn(
                4.0f,
                [this, y, startZ, endZ]()
                {
                    hazardManager->SpawnLinearHazard(
                        HazardType::RollingRock,
                        glm::vec3(-2.0f, y, startZ),
                        glm::vec3(-2.0f, y, endZ),
                        1.2f,
                        false);
                });

            hazardManager->RegisterRepeatingSpawn(
                3.0f,
                [this, y, startZ, endZ]()
                {
                    hazardManager->SpawnLinearHazard(
                        HazardType::RollingLog,
                        glm::vec3(2.0f, y, startZ),
                        glm::vec3(2.0f, y, endZ),
                        2.2f,
                        false);
                });
        }
    }

    // =========================================================
    // REMOVED: The Cow
    // =========================================================
    /*
    {
        const int row = level->GetSpawnRow() + 15;

        if (const Lane* lane = level->GetLane(row))
        {
           hazardManager->SpawnCow(
               glm::vec3(halfWidth * 0.6f, lane->GetSurfaceHeight(), lane->GetCenterZ()),
               2.5f);
        }
    }
    */
}

void Game::SpawnExampleCollectibles()
{
    if (!level || !collectibleManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // One of each ally, spaced out through the safe lanes near the start
    const PieceType allies[4] =
    {
        PieceType::Bishop,
        PieceType::Knight,
        PieceType::Rook,
        PieceType::Queen
    };

    for (int index = 0; index < 4; ++index)
    {
        const int row = level->GetSpawnRow() + 1 + index * 2;

        if (const Lane* lane = level->GetLane(row))
        {
            // MOVED TO CENTER (x = 0.0f) so you walk straight into them
            collectibleManager->Spawn(
                allies[index],
                glm::vec3(0.0f, lane->GetSurfaceHeight(), lane->GetCenterZ()));
        }
    }
}

void Game::UpdateCamera()
{
    if (!camera || !pawn)
        return;

    // Aim a little past the pawn so slightly more of the road ahead is
    // visible than the ground already crossed.
    const glm::vec3 lookAhead(
        0.0f,
        0.0f,
        -GameConfig::CameraLookAhead);

    camera->SetTarget(
        pawn->GetTransform().GetPosition() + lookAhead);
}

void Game::Update(float deltaTime)
{
    if (level)
        level->Update(deltaTime);

    if (pawn && pawn->IsActive())
        pawn->Update(deltaTime);

    if (hazardManager && pawn)
        hazardManager->Update(deltaTime, pawn->GetTransform().GetPosition());

    // ---- Update Hazard Collision ----
    if (hazardCollision)
    {
        hazardCollision->Update(
            hazardManager->GetHazards(),
            stationaryHazards,
            deltaTime);
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

    UpdateCamera();
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

    // ---------------------------------------------------------
    // REMOVED: showcasePieces and showcaseObstacles shadows
    // ---------------------------------------------------------

    // Stationary hazard shadows
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

    // The frame is the cage's root mesh. The barred door is deliberately a
    // second object with its origin on the hinge.
    if (kingsCage)
        renderer->Draw(*kingsCage);

    if (capturedKing)
        renderer->Draw(*capturedKing);

    if (kingsCage)
    {
        if (WorldObject* door = kingsCage->GetDoor())
            renderer->Draw(*door);
    }

    // ---------------------------------------------------------
    // REMOVED: showcasePieces and showcaseObstacles models
    // ---------------------------------------------------------

    // Stationary hazards (spikes, walls, fences, trees, rocks, bushes, palisades)
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
        {
            if (piece)
                renderer->Draw(*piece);
        }
    }

    if (pawn)
        renderer->Draw(*pawn);

    // Sprites last, after every opaque mesh.
    if (spriteRenderer)
        spriteRenderer->DrawAll(sprites);
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
    showcasePieces.clear();
    showcaseObstacles.clear();
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