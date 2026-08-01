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

#include "GameConfig.h"
#include "ResourceManager.h"
#include "SceneNode.h"
#include "Shader.h"

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

    level = std::make_shared<Level>();
    level->Build();

    pawn = std::make_shared<Pawn>();
    pawn->SetSpawnPosition(level->GetPlayerSpawnPosition());
    pawn->Initialize();

    // NOTE(Ayub): required for the pawn's free movement to follow each
    // lane's terrain height. Non-owning; Level outlives Pawn here.
    pawn->SetLevel(level.get());

    pieceMeshes = std::make_shared<PieceMeshLibrary>();

    // After the mesh library exists, and after the pawn: the rig shares the
    // library's meshes, so neither can be created before the other.
    pawn->AttachRig(*pieceMeshes);
    obstacleMeshes = std::make_shared<ObstacleMeshLibrary>();
    gateMeshes = std::make_shared<GateMeshLibrary>();
    cageMeshes = std::make_shared<CageMeshLibrary>();

    BuildCheckpointGate();
    BuildKingsCage();

    hazardManager = std::make_shared<HazardManager>(*obstacleMeshes);
    collectibleManager = std::make_shared<CollectibleManager>(*pieceMeshes);

    BuildShowcase();
    SpawnExampleHazards();
    SpawnExampleCollectibles();

    // Frame the pawn before the first frame is drawn, so it is already in
    // view the moment the window opens.
    UpdateCamera();

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
    if (!level || !pieceMeshes || !obstacleMeshes)
        return;

    showcasePieces.clear();
    showcaseObstacles.clear();

    for (int index = 0; index < PieceTypeCount; ++index)
    {
        auto piece = pieceMeshes->CreatePiece(
            PieceTypeFromIndex(index),
            PieceTeam::White);

        piece->SetGroundPosition(
            GetShowcaseSlot(
                PieceRowLanesAhead,
                index,
                PieceTypeCount,
                PieceSpacing));

        piece->SetHeadingDegrees(
            PieceDisplayHeadingDegrees(piece->GetType()));

        showcasePieces.push_back(piece);
    }

    // Stationary props, in the order the enum lists them. That order puts
    // the tree and the palisade - the only two tall enough to hide the row
    // behind them - at the far ends, clear of the hazards.
    for (int index = 0; index < ObstacleTypeCount; ++index)
    {
        auto obstacle = obstacleMeshes->CreateObstacle(
            ObstacleTypeFromIndex(index));

        glm::vec3 position = GetShowcaseSlot(
            ObstacleRowLanesAhead,
            index,
            ObstacleTypeCount,
            ObstacleSpacing);

        // The cow faces the palisade and its head projects beyond its body's
        // logical origin. Give that pair an intentional gap while keeping
        // both outer assets clear of the decorative border.
        if (obstacle->GetType() == ObstacleType::Cow)
            position.x -= 0.12f;

        obstacle->SetGroundPosition(position);

        showcaseObstacles.push_back(obstacle);
    }

    // Hazards last, furthest away: their models are the shortest, and the
    // far end of the view has the least vertical room. Fireball and Lightning
    // are logical gameplay types only until their sprite visuals arrive, so
    // they are intentionally absent from this 3D showcase.
    std::vector<std::shared_ptr<Hazard>> showcaseHazards;

    for (int index = 0; index < HazardTypeCount; ++index)
    {
        auto hazard = obstacleMeshes->CreateHazard(
            HazardTypeFromIndex(index));

        if (hazard && hazard->GetMesh())
            showcaseHazards.push_back(hazard);
    }

    const int showcaseHazardCount =
        static_cast<int>(showcaseHazards.size());

    for (int index = 0; index < showcaseHazardCount; ++index)
    {
        auto& hazard = showcaseHazards[index];

        hazard->SetGroundPosition(
            GetShowcaseSlot(
                HazardRowLanesAhead,
                index,
                showcaseHazardCount,
                HazardSpacing));

        showcaseObstacles.push_back(hazard);
    }
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

    // Centred on the board and standing on the checkpoint lane's own
    // surface, so the wall run crosses the level exactly where the GDD puts
    // the checkpoint rather than at an assumed height.
    checkpointGate->SetGroundPosition(
        glm::vec3(
            0.0f,
            lane->GetSurfaceHeight(),
            lane->GetCenterZ()));
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
    // NOTE(Ayub/Liyuu): no dedicated Spear lane exists in Level 1 yet --
    // placed in a SafeGrass row purely so it's visible for testing.
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
    // sideways across it like arrows/cannonballs. Neither has a
    // dedicated lane in Level 1 yet, so these roll through a short span
    // of SafeGrass rows purely for testing.
    {
        // Keep the moving previews beyond the three static showcase rows
        // (+2, +4, +6), so they demonstrate motion without passing through
        // the obstacle and hazard displays.
        const int startRow = level->GetSpawnRow() + 7;
        const int endRow = level->GetSpawnRow() + 9;

        if (const Lane* startLane = level->GetLane(startRow))
        {
            const float y = startLane->GetSurfaceHeight();
            const float startZ = Level::RowToWorldZ(startRow);
            const float endZ = Level::RowToWorldZ(endRow);

            // "A slow vertical projectile with a large hit area."
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

            // "Similar to the rolling rock, but faster."
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

    // Cow: chases the pawn indefinitely from just ahead of the start area.
    // It follows continuously on its own, so it needs no repeat spawner.
    {
        const int row = level->GetSpawnRow() + 15;

        if (const Lane* lane = level->GetLane(row))
        {
            hazardManager->SpawnCow(
                glm::vec3(halfWidth * 0.6f, lane->GetSurfaceHeight(), lane->GetCenterZ()),
                2.5f);
        }
    }
}

void Game::SpawnExampleCollectibles()
{
    if (!level || !collectibleManager)
        return;

    const float halfWidth = Level::GetPlayableHalfWidth();

    // One of each ally, spaced out through the safe lanes near the start
    // so all four are easy to test without dodging every hazard first.
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
            const float x = (index % 2 == 0)
                ? halfWidth * 0.35f
                : -halfWidth * 0.35f;

            collectibleManager->Spawn(
                allies[index],
                glm::vec3(x, lane->GetSurfaceHeight(), lane->GetCenterZ()));
        }
    }

    // Same presentation pass as the showcase: the allies stand still until
    // they are picked up, so the horse-bodied ones are shown in profile.
    for (const auto& piece : collectibleManager->GetCollectibles())
    {
        if (piece)
        {
            piece->SetHeadingDegrees(
                PieceDisplayHeadingDegrees(piece->GetType()));
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
    if (!kingsCage || !kingsCage->GetDoor())
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
        if (WorldObject* door = kingsCage->GetDoor())
        {
            kingsCageDoorAngle = std::min(
                kingsCageDoorAngle + GameConfig::DoorOpenSpeed * deltaTime,
                GameConfig::KingsCageMaxDoorAngle);

            // The door's origin is authored on its own hinge (see
            // CageMetrics::DoorHingeX/Y/Z), so a plain Y rotation is the
            // entire animation, the same way CheckpointGate's leaves work.
            door->GetTransform().SetRotation(
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

    if (pawn && pawn->IsActive())
        pawn->Update(deltaTime);

    // Animated pieces need their own tick. Nothing on the field moves them
    // yet, so they hold the rest pose - but the moment something does call
    // SetMovementState on one, it walks, with no further wiring here.
    for (const auto& piece : showcasePieces)
    {
        if (piece && piece->IsActive())
            piece->Update(deltaTime);
    }

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

    // TEMPORARY: stands in for Kaung's real collision detection. Delete
    // once his system calls Pawn::CollectPiece(...) directly on overlap.
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

    // Bishop's ability mutates the world (removing nearby hazards), which
    // is why Pawn only raises a pulse instead of doing this itself -- see
    // the note on ConsumeBishopActivationPulse().
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

    // Every shadow before every model: shadows are translucent and have to
    // blend over ground that has already been drawn.
    for (const auto& piece : showcasePieces)
    {
        if (piece)
            renderer->Draw(piece->GetShadow());
    }

    for (const auto& obstacle : showcaseObstacles)
    {
        if (obstacle)
            renderer->Draw(obstacle->GetShadow());
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

    // The gate is several meshes rather than one, because its two leaves
    // turn on their own hinges. The renderer neither knows nor cares: each
    // part is an ordinary WorldObject carrying its own transform.
    if (checkpointGate)
    {
        for (const auto& part : checkpointGate->GetParts())
        {
            if (part)
                renderer->Draw(*part);
        }
    }

    // The frame is the cage's root mesh. The barred door is deliberately a
    // second object with its origin on the hinge, ready for a future rescue
    // animation without changing either mesh.
    if (kingsCage)
        renderer->Draw(*kingsCage);

    if (capturedKing)
        renderer->Draw(*capturedKing);

    if (kingsCage)
    {
        if (WorldObject* door = kingsCage->GetDoor())
            renderer->Draw(*door);
    }

    for (const auto& piece : showcasePieces)
        DrawPiece(*piece);

    for (const auto& obstacle : showcaseObstacles)
    {
        if (obstacle)
            renderer->Draw(*obstacle);
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
    // them would ignore them and punch straight through. DrawAll brackets its
    // own GL state and returns immediately while the list is empty, which it
    // is until sprite assets exist.
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
