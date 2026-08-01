/*
    ============================================================
    Checkmate Crossing - Pawn

    The player's piece: free top-down movement (GDD section 2), clamped
    to the playable width and following the terrain height of whatever
    lane it currently stands over.

    Collision response (blocking, damage, knockback) is out of scope
    here by design -- see Pawn.h for the ownership split with Kaung's
    collision system.
    ============================================================
*/

#include "Pawn.h"

#include <cmath>

#include "GameConfig.h"
#include "Input.h"
#include "Level.h"
#include "PieceAnimator.h"
#include "PieceMeshFactory.h"
#include "PieceRig.h"

namespace
{
    /// An empty list to hand back while the pawn is still a cube.
    const std::vector<std::shared_ptr<SceneNode>>& NoParts()
    {
        static const std::vector<std::shared_ptr<SceneNode>> empty;

        return empty;
    }

    /// How far above its lane the pawn has to be before it counts as
    /// airborne. Generous enough to swallow the float noise in the terrain
    /// snap, small enough that any real hop clears it.
    constexpr float GroundedTolerance = 0.02f;
}

namespace
{
    /// Reads WASD/arrow input into a normalized XZ direction, so moving
    /// diagonally isn't faster than moving along a single axis. Matches
    /// the control table in GDD section 2.
    glm::vec3 ReadMoveDirection()
    {
        glm::vec3 direction(0.0f);

        if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up))
            direction.z -= 1.0f;

        if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down))
            direction.z += 1.0f;

        if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left))
            direction.x -= 1.0f;

        if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right))
            direction.x += 1.0f;

        if (glm::length(direction) > 0.0f)
            direction = glm::normalize(direction);

        return direction;
    }
}

Pawn::Pawn()
    : spawnPosition(0.0f),
    groundHeight(0.0f)
{
}

Pawn::~Pawn() = default;

void Pawn::Initialize()
{
    SetMesh(Mesh::CreateCube());

    SetColor(GameConfig::PawnColor);

    shadow.SetFootprint(GameConfig::PawnWidth * GameConfig::ShadowScale);
    shadow.Initialize();

    // Slightly taller than it is wide, so the piece reads as standing up
    // rather than as a floor tile.
    transform.SetScale(
        GameConfig::PawnWidth,
        GameConfig::PawnHeight,
        GameConfig::PawnWidth);

    transform.SetRotation(0.0f, 0.0f, 0.0f);

    Respawn();
}

void Pawn::Update(float deltaTime)
{
    HandleInput();

    UpdateAbility(deltaTime);

    glm::vec3 position = transform.GetPosition();
    position += velocity * deltaTime;

    // Z is intentionally left unclamped here: the pawn is free to walk the
    // full length of the level, including back toward the start. Kaung's
    // checkpoint/respawn logic is the right place to prevent backtracking
    // past a cleared checkpoint, if that turns out to be desired -- it
    // isn't a movement concern.
    const float halfWidth = Level::GetPlayableHalfWidth();
    position.x = glm::clamp(position.x, -halfWidth, halfWidth);

    transform.SetPosition(position);

    ApplyTerrainHeight();

    UpdateShadow();

    // Last, and reading only what movement has already settled. Animation is
    // downstream of the gameplay here and never the other way round.
    UpdateAnimation(deltaTime);
}

void Pawn::HandleInput()
{
    const glm::vec3 direction = ReadMoveDirection();

    velocity = direction * moveSpeed * GetSpeedMultiplier();

    if (glm::length(direction) > 0.0f)
    {
        // Y-heading toward the movement direction.
        //
        // Every piece model is authored facing +Z (see PieceMeshFactory),
        // and a Y rotation of theta carries +Z round to
        // (sin theta, 0, cos theta). Pointing that at the travel direction
        // is therefore atan2(x, z), with no negation on either term.
        //
        // The negated Z this replaced dated from when the pawn was a cube
        // with no visible front, and inverted forward against backward
        // while leaving left and right correct - which is exactly how the
        // bug presented once the cube became a real model.
        const float headingDegrees =
            glm::degrees(std::atan2(direction.x, direction.z));

        transform.SetRotation(0.0f, headingDegrees, 0.0f);
    }

    const bool spaceDown = Input::IsKeyPressed(Key::Space);

    if (spaceDown && !spaceKeyWasDown)
        TryActivateAbility();

    spaceKeyWasDown = spaceDown;
}

void Pawn::ApplyTerrainHeight()
{
    if (!level)
        return;

    const glm::vec3 position = transform.GetPosition();

    // Level::RowToWorldZ(row) = -row * TileSize; inverted here to find
    // which row the pawn's current Z falls closest to.
    const int row = static_cast<int>(
        std::lround(-position.z / GameConfig::TileSize));

    const Lane* lane = level->GetLane(row);

    if (!lane)
        return;

    groundHeight = lane->GetSurfaceHeight();

    glm::vec3 adjusted = position;
    adjusted.y = groundHeight + GameConfig::PawnHeight * 0.5f;

    transform.SetPosition(adjusted);
}

void Pawn::AttachRig(PieceMeshLibrary& meshLibrary)
{
    const PieceMeshFactory::PieceRigModel& model =
        meshLibrary.GetRigModel(PieceType::Pawn);

    if (!model.valid)
        return;

    rig = std::make_unique<PieceRig>();
    rig->Build(model);
    rig->SetScale(GameConfig::PieceScale);

    animator = std::make_unique<PieceAnimator>(PieceBodyPlan::Humanoid);

    // The cube goes, but nothing else does: PawnWidth and PawnHeight still
    // drive the shadow, the terrain snap and every distance check in the
    // game. The rig is only what the player sees.
    SetMesh(nullptr);

    UpdateAnimation(0.0f);
}

const std::vector<std::shared_ptr<SceneNode>>& Pawn::GetRigParts() const
{
    return rig ? rig->GetParts() : NoParts();
}

bool Pawn::IsGrounded() const
{
    // ApplyTerrainHeight puts the pawn's centre exactly half its height
    // above the lane, so anything meaningfully above that is off the ground.
    const float restingY = groundHeight + GameConfig::PawnHeight * 0.5f;

    return transform.GetPosition().y <= restingY + GroundedTolerance;
}

void Pawn::UpdateAnimation(float deltaTime)
{
    if (!rig || !animator)
        return;

    animator->Update(
        deltaTime,
        IsMoving(),
        IsGrounded(),
        GetSpeedMultiplier());

    // The rig stands on the ground the pawn is over, not at the pawn's
    // centre: the models are authored with y = 0 at the feet, and the
    // transform is the middle of a cube that no longer exists.
    const glm::vec3& position = transform.GetPosition();

    rig->SetGroundPosition(
        glm::vec3(position.x, groundHeight, position.z));

    rig->SetHeadingDegrees(transform.GetRotation().y);

    rig->ApplyPose(animator->GetPose());
}

void Pawn::CollectPiece(PieceType type)
{
    // Only the four collectible allies make sense here; Pawn/King/
    // MountedPawn are never placed as field collectibles.
    if (type != PieceType::Bishop &&
        type != PieceType::Knight &&
        type != PieceType::Rook &&
        type != PieceType::Queen)
    {
        return;
    }

    // Only one banked ability at a time, matching the GDD's single
    // "stored chess ability" slot -- picking up a new one overwrites
    // whatever was previously held and unused.
    hasStoredPiece = true;
    storedPieceType = type;
}

bool Pawn::HasStoredPiece() const
{
    return hasStoredPiece;
}

PieceType Pawn::GetStoredPieceType() const
{
    return storedPieceType;
}

void Pawn::TryActivateAbility()
{
    if (!hasStoredPiece)
        return;

    activeAbilityType = storedPieceType;
    hasStoredPiece = false;

    switch (activeAbilityType)
    {
    case PieceType::Knight:

        // Speed boost and hazard immunity for a fixed duration.
        abilityActive = true;
        abilityTimeRemaining = GameConfig::KnightAbilityDuration;
        shieldAvailable = false;
        break;

    case PieceType::Rook:

        // A single shield charge; ConsumeShield() ends this early once
        // it actually blocks a hit.
        abilityActive = true;
        abilityTimeRemaining = GameConfig::RookShieldDuration;
        shieldAvailable = true;
        break;

    case PieceType::Queen:

        // Knight's speed/immunity plus Rook's shield together, for a
        // shorter duration -- the GDD's "combines multiple abilities."
        abilityActive = true;
        abilityTimeRemaining = GameConfig::QueenAbilityDuration;
        shieldAvailable = true;
        break;

    case PieceType::Bishop:

        // Instant effect: no ongoing timer, just a one-shot pulse for
        // whoever owns HazardManager (currently Game) to react to.
        abilityActive = false;
        bishopPulsePending = true;
        break;

    default:

        // Pawn/King/MountedPawn are never valid collectibles; CollectPiece
        // already filters these out, so this shouldn't be reachable.
        break;
    }
}

void Pawn::UpdateAbility(float deltaTime)
{
    if (!abilityActive)
        return;

    abilityTimeRemaining -= deltaTime;

    if (abilityTimeRemaining <= 0.0f)
    {
        abilityActive = false;
        shieldAvailable = false;
    }
}

bool Pawn::IsAbilityActive() const
{
    return abilityActive;
}

PieceType Pawn::GetActiveAbilityType() const
{
    return activeAbilityType;
}

float Pawn::GetSpeedMultiplier() const
{
    if (abilityActive &&
        (activeAbilityType == PieceType::Knight ||
            activeAbilityType == PieceType::Queen))
    {
        return GameConfig::KnightSpeedMultiplier;
    }

    return 1.0f;
}

bool Pawn::IsImmuneToHazards() const
{
    return abilityActive &&
        (activeAbilityType == PieceType::Knight ||
            activeAbilityType == PieceType::Queen);
}

bool Pawn::HasShield() const
{
    return abilityActive && shieldAvailable;
}

void Pawn::ConsumeShield()
{
    shieldAvailable = false;

    // A consumed Rook shield was the entire ability, so it ends outright.
    // A consumed Queen shield only drops the shield half of her kit --
    // her speed and immunity continue until her own timer runs out.
    if (activeAbilityType == PieceType::Rook)
    {
        abilityActive = false;
        abilityTimeRemaining = 0.0f;
    }
}

bool Pawn::ConsumeBishopActivationPulse()
{
    if (!bishopPulsePending)
        return false;

    bishopPulsePending = false;
    return true;
}

void Pawn::SetLevel(const Level* level)
{
    this->level = level;
}

float Pawn::GetMoveSpeed() const
{
    return moveSpeed;
}

void Pawn::SetMoveSpeed(float unitsPerSecond)
{
    moveSpeed = unitsPerSecond;
}

const glm::vec3& Pawn::GetVelocity() const
{
    return velocity;
}

bool Pawn::IsMoving() const
{
    return glm::length(velocity) > 0.0001f;
}

int Pawn::GetCurrentRow() const
{
    return static_cast<int>(
        std::lround(-transform.GetPosition().z / GameConfig::TileSize));
}

void Pawn::SetSpawnPosition(const glm::vec3& groundPosition)
{
    spawnPosition = groundPosition;
}

const glm::vec3& Pawn::GetSpawnPosition() const
{
    return spawnPosition;
}

void Pawn::Respawn()
{
    // The spawn point is a point on the ground, and the cube mesh is centred
    // on its own origin, so lifting it by half its height rests its base
    // exactly on that surface.
    groundHeight = spawnPosition.y;

    transform.SetPosition(
        spawnPosition.x,
        spawnPosition.y + GameConfig::PawnHeight * 0.5f,
        spawnPosition.z);

    UpdateShadow();
}

const WorldObject& Pawn::GetShadow() const
{
    return shadow;
}

void Pawn::UpdateShadow()
{
    const glm::vec3& position = transform.GetPosition();

    shadow.PlaceOn(
        glm::vec3(position.x, groundHeight, position.z));
}
