/*
    ============================================================
    Checkmate Crossing - Pawn

    The player's piece: free top-down movement (GDD section 2), clamped
    to the playable width and following the terrain height of whatever
    lane it currently stands over, plus a jump (Space) for clearing
    Fence/Rock/Palisade and an interact button (E) whose actual target
    (door, gate, or falling back to the banked ability) is resolved by
    whoever owns both the pawn and those objects -- currently Game.

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

    UpdateJump(deltaTime);

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
}

void Pawn::HandleInput()
{
    const glm::vec3 direction = ReadMoveDirection();

    velocity = direction * moveSpeed * GetSpeedMultiplier();

    if (glm::length(direction) > 0.0f)
    {
        // Y-heading toward the movement direction; 0 degrees faces -Z
        // (toward the king), matching the direction of progress. The cube
        // placeholder has no visible "front" yet, so the sign here is a
        // best guess -- flip it if a directional pawn model ends up facing
        // backward once John's real model/animations are in.
        const float headingDegrees =
            glm::degrees(std::atan2(direction.x, -direction.z));

        transform.SetRotation(0.0f, headingDegrees, 0.0f);
    }

    const bool spaceDown = Input::IsKeyPressed(Key::Space);

    if (spaceDown && !spaceKeyWasDown)
        TryJump();

    spaceKeyWasDown = spaceDown;

    // E is the interact button. What it actually does -- open a nearby
    // door/gate, or fall back to activating a stored ability -- depends on
    // the wider level, which this class has no reference to. It just
    // reports the press; see the class comment on ConsumeInteractPulse().
    const bool interactDown = Input::IsKeyPressed(Key::E);

    if (interactDown && !interactKeyWasDown)
        interactPulsePending = true;

    interactKeyWasDown = interactDown;
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
    adjusted.y = groundHeight + GameConfig::PawnHeight * 0.5f + jumpHeight;

    transform.SetPosition(adjusted);
}

void Pawn::TryJump()
{
    // Only from the ground -- no double-jumping, and a jump started mid-
    // jump would just reset the arc rather than adding to it.
    if (isAirborne)
        return;

    isAirborne = true;
    jumpVerticalVelocity = GameConfig::JumpInitialVelocity;
}

void Pawn::UpdateJump(float deltaTime)
{
    if (!isAirborne)
        return;

    jumpVerticalVelocity -= GameConfig::JumpGravity * deltaTime;
    jumpHeight += jumpVerticalVelocity * deltaTime;

    if (jumpHeight <= 0.0f)
    {
        jumpHeight = 0.0f;
        jumpVerticalVelocity = 0.0f;
        isAirborne = false;
    }
}

bool Pawn::IsAirborne() const
{
    return isAirborne;
}

float Pawn::GetJumpHeight() const
{
    return jumpHeight;
}

bool Pawn::ConsumeInteractPulse()
{
    if (!interactPulsePending)
        return false;

    interactPulsePending = false;
    return true;
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

    // Clears any jump in progress -- respawning mid-air (e.g. after a hit
    // while jumping a Palisade) should land the pawn cleanly, not carry
    // the old arc's velocity into the new position.
    isAirborne = false;
    jumpHeight = 0.0f;
    jumpVerticalVelocity = 0.0f;

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
