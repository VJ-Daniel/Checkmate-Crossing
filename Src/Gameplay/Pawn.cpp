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
#include <iostream> // Added for debug output

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

    // Store the base scale so we can revert to it if needed
    pawnBaseScale = transform.GetScale();

    Respawn();
}

void Pawn::Update(float deltaTime)
{
    // Update slow timer
    if (slowTimer > 0.0f)
    {
        slowTimer -= deltaTime;
        if (slowTimer <= 0.0f)
            slowMultiplier = 1.0f;
    }

    // Update knockback timer
    if (knockbackTimer > 0.0f)
    {
        knockbackTimer -= deltaTime;
    }

    HandleInput();
    UpdateAbility(deltaTime);

    glm::vec3 position = transform.GetPosition();

    // Apply slow to velocity before movement
    glm::vec3 finalVelocity = velocity * slowMultiplier;
    position.x += finalVelocity.x * deltaTime;
    position.z += finalVelocity.z * deltaTime;

    const float halfWidth = Level::GetPlayableHalfWidth();
    position.x = glm::clamp(position.x, -halfWidth, halfWidth);

    transform.SetPosition(position);

    ApplyTerrainHeight(); // Simply snaps Y to ground since we removed jumping

    UpdateShadow();
}

// --- NEW: SET THE MESH LIBRARY ---
void Pawn::SetMeshLibrary(std::shared_ptr<PieceMeshLibrary> library)
{
    meshLibrary = library;
}
// ---------------------------------

void Pawn::HandleInput()
{
    const glm::vec3 direction = ReadMoveDirection();

    // FIX: Only apply input velocity if we aren't currently being knocked back!
    if (knockbackTimer <= 0.0f)
    {
        velocity = direction * moveSpeed * GetSpeedMultiplier();
    }
    else
    {
        // If we are in knockback, slowly reduce the bounce so it smoothly stops
        velocity *= 0.95f;
        if (glm::length(velocity) < 0.01f) velocity = glm::vec3(0.0f);
    }

    if (glm::length(direction) > 0.0f && knockbackTimer <= 0.0f)
    {
        // Y-heading toward the movement direction
        const float headingDegrees = glm::degrees(std::atan2(direction.x, -direction.z));
        transform.SetRotation(0.0f, headingDegrees, 0.0f);
    }

    const bool spaceDown = Input::IsKeyPressed(Key::Space);

    // SPACE NOW ONLY ACTIVATES ABILITIES (NO JUMPING)
    if (spaceDown && !spaceKeyWasDown)
    {
        TryActivateAbility();
    }

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

    // =========================================
    // Now it ALWAYS snaps to the ground!
    // =========================================
    adjusted.y = groundHeight + GameConfig::PawnHeight * 0.5f;

    transform.SetPosition(adjusted);
}

void Pawn::CollectPiece(PieceType type)
{
    // Only the four collectible allies make sense here.
    if (type != PieceType::Bishop &&
        type != PieceType::Knight &&
        type != PieceType::Rook &&
        type != PieceType::Queen)
    {
        return;
    }

    // Store the ability
    hasStoredPiece = true;
    storedPieceType = type;

    // =========================================================
    // THE FIX: SWAP THE MESH TO THE COLLECTED PIECE!
    // =========================================================
    if (meshLibrary)
    {
        // Create a temporary chess piece model
        auto newMesh = meshLibrary->CreatePiece(type, PieceTeam::White);

        // Steal its mesh and apply it to our pawn
        SetMesh(newMesh->GetMesh());

        // Set the team color of the piece
        SetColor(newMesh->GetColor());

        // Resize it to fit the pawn's scale
        float pieceScale = GameConfig::PieceScale;
        transform.SetScale(pieceScale, pieceScale, pieceScale);
    }
    // =========================================================

    std::cout << "Pawn transformed into a " << GetPieceTypeName(type) << "!" << std::endl;
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

    // --- RESET HEALTH ON RESPAWN ---
    health = 5.0f;
    // --- CLEAR KNOCKBACK ---
    velocity = glm::vec3(0.0f);
    knockbackTimer = 0.0f;
    // ------------------------------

    // =========================================
    // REVERT MESH, SIZE, AND COLOR BACK TO NORMAL
    // =========================================
    SetMesh(Mesh::CreateCube()); // <-- Change back to a cube
    SetColor(GameConfig::PawnColor); // Revert to original white color
    transform.SetScale(
        GameConfig::PawnWidth,
        GameConfig::PawnHeight,
        GameConfig::PawnWidth
    );
    // =========================================

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

void Pawn::ApplySlow(float duration, float amount)
{
    float newMultiplier = 1.0f - amount;
    if (newMultiplier < slowMultiplier) slowMultiplier = newMultiplier;
    slowTimer = std::max(slowTimer, duration);

    // DEBUG LINE:
    std::cout << "Slow Applied! Multiplier: " << slowMultiplier << " | Timer: " << slowTimer << "s" << std::endl;
}

// --- TAKE DAMAGE FUNCTION ---
void Pawn::TakeDamage(float damage)
{
    health -= damage;

    // Print to console for debugging
    std::cout << "Pawn took " << damage << " damage! HP: " << health << std::endl;

    if (health <= 0.0f)
    {
        health = 0.0f;
        std::cout << "Pawn has died! Respawning..." << std::endl;
        Respawn();
    }
}

void Pawn::SetKnockback(const glm::vec3& knockbackVector, bool isCow /* = false */)
{
    // This function is called by the collision system via the onKnockback callback.
    velocity += knockbackVector;
    knockbackTimer = 0.35f; // Prevent input for 0.35 seconds

    // Store who knocked us back
    knockedBackByCow = isCow;
}