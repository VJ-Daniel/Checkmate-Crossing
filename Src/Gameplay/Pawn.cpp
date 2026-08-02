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
#include <iostream> // Added for debug output

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

    // Store the base scale so we can revert to it if needed
    pawnBaseScale = transform.GetScale();

    Respawn();
}

void Pawn::Update(float deltaTime)
{
    // Mud and bushes slow the pawn for a while after it walks through them.
    if (slowTimer > 0.0f)
    {
        slowTimer -= deltaTime;

        if (slowTimer <= 0.0f)
            slowMultiplier = 1.0f;
    }

    // A hit pushes the pawn for a moment, during which input is ignored.
    if (knockbackTimer > 0.0f)
        knockbackTimer -= deltaTime;

    HandleInput();

    UpdateJump(deltaTime);

    UpdateAbility(deltaTime);

    glm::vec3 position = transform.GetPosition();

    // The slow scales the whole step rather than the stored velocity, so
    // GetVelocity still reports what the pawn is trying to do and the
    // animation stride does not stall along with it.
    position += velocity * slowMultiplier * deltaTime;

    // Resolve the complete X/Z footprint against one continuous playable
    // rectangle. This is equivalent to four gapless boundary colliders, but
    // uses the clamp already owned by movement instead of adding inert
    // collider objects to a project that has no generic resolver yet.
    //
    // Y is untouched, so jumping cannot hop over a boundary and the jump arc
    // never gets flattened by horizontal collision response.
    if (level)
    {
        // The current character's own half-width, so a switch to a wider
        // body is resolved against the bounds correctly.
        const float halfFootprint = (footprintRadius > 0.0f)
            ? footprintRadius
            : GameConfig::PawnWidth * 0.5f;

        const glm::vec3 resolved = level->ClampToPlayableBounds(
            position,
            halfFootprint,
            halfFootprint);

        // Remove only the velocity normal to a wall. Diagonal movement keeps
        // its tangent component and slides naturally along sides/corners.
        if (resolved.x != position.x)
            velocity.x = 0.0f;

        if (resolved.z != position.z)
            velocity.z = 0.0f;

        position = resolved;
    }
    else
    {
        // Pawn is normally attached to a Level before its first Update, but
        // retain a safe width clamp for isolated tests or placeholder use.
        const float halfWidth =
            Level::GetPlayableHalfWidth() - GameConfig::PawnWidth * 0.5f;

        position.x = glm::clamp(position.x, -halfWidth, halfWidth);
    }

    transform.SetPosition(position);

    ApplyTerrainHeight();

    UpdateShadow();

    // Last, and reading only what movement has already settled. Animation is
    // downstream of the gameplay here and never the other way round.
    UpdateAnimation(deltaTime);
}


void Pawn::SetMeshLibrary(std::shared_ptr<PieceMeshLibrary> library)
{
    meshLibrary = library;
}
// ---------------------------------

void Pawn::HandleInput()
{
    const glm::vec3 direction = ReadMoveDirection();

    // Recorded before collision gets a say, so the walk cycle keeps
    // running while the pawn is held against a wall.
    movementInputActive = glm::length(direction) > 0.0f;

    // Knockback overrides input while it lasts, so a hit actually pushes the
    // pawn instead of being cancelled by whatever key is held.
    if (knockbackTimer <= 0.0f)
    {
        velocity = direction * moveSpeed * GetSpeedMultiplier();
    }
    else
    {
        // Bleed the bounce off smoothly rather than stopping dead.
        velocity *= 0.95f;

        if (glm::length(velocity) < 0.01f)
            velocity = glm::vec3(0.0f);
    }

    if (glm::length(direction) > 0.0f && knockbackTimer <= 0.0f)
    {
        // Y-heading toward the movement direction.
        //
        // Every piece model is authored facing +Z (see PieceMeshFactory),
        // and a Y rotation of theta carries +Z round to
        // (sin theta, 0, cos theta). Pointing that at the travel direction
        // is therefore atan2(x, z), with no negation on either term.
        //
        // The negated Z is the orientation bug that was fixed after Kaung
        // branched: it inverted forward against backward while leaving left
        // and right correct.
        const float headingDegrees =
            glm::degrees(std::atan2(direction.x, direction.z));

        transform.SetRotation(0.0f, headingDegrees, 0.0f);
    }

    // Space jumps. Kaung's branch had repurposed it to fire abilities,
    // because it predates the jump landing on the shared branch - but E
    // already resolves abilities (see ConsumeInteractPulse), and removing
    // the jump would take Ayub's movement and the jump animations with it.
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

void Pawn::AttachRig(PieceMeshLibrary& meshLibrary)
{
    SetCharacter(PieceType::Pawn, meshLibrary);
}

bool Pawn::GetCharacterAbility(
    PieceType character,
    PieceType& abilityType)
{
    switch (character)
    {
        // The three collectible allies map straight onto their own
        // abilities, which the ability system already implements.
    case PieceType::Bishop:
    case PieceType::Rook:
    case PieceType::Queen:
        abilityType = character;
        return true;

        // Riding the horse is what grants the horse's speed and hazard
        // immunity, so the mounted character banks the Knight ability. The
        // standalone Knight is not a playable character.
    case PieceType::MountedPawn:
        abilityType = PieceType::Knight;
        return true;

        // The plain pawn has no ability by design, and the King has none
        // implemented anywhere in the project or written down in the GDD.
    default:
        return false;
    }
}

void Pawn::ClearAbilityState()
{
    abilityActive = false;
    abilityTimeRemaining = 0.0f;
    shieldAvailable = false;
    hasStoredPiece = false;
    bishopPulsePending = false;
}

void Pawn::SetCharacter(
    PieceType newCharacter,
    PieceMeshLibrary& meshLibrary)
{
    // The riderless horse is a collectible, not a body the player wears.
    if (newCharacter == PieceType::Knight)
        return;

    const PieceMeshFactory::PieceRigModel& rigModel =
        meshLibrary.GetRigModel(newCharacter);

    if (!rigModel.valid)
        return;

    character = newCharacter;

    // Replacing the unique_ptr releases the previous character's parts, so
    // only one body is ever in the draw list.
    rig = std::make_unique<PieceRig>();
    rig->Build(rigModel);
    rig->SetScale(GameConfig::PieceScale);

    const bool quadruped = (newCharacter == PieceType::MountedPawn);

    animator = std::make_unique<PieceAnimator>(
        quadruped ? PieceBodyPlan::Quadruped : PieceBodyPlan::Humanoid);

    // Footprint and shadow come from the model the character is actually
    // built from, so both follow the body rather than staying on the pawn's.
    const PieceModel& baked = meshLibrary.GetModel(newCharacter);

    const float width = baked.baseWidth * GameConfig::PieceScale;
    const float depth = baked.baseDepth * GameConfig::PieceScale;

    footprintRadius = width * 0.5f;

    shadow.SetFootprint(
        width * GameConfig::ShadowScale,
        depth * GameConfig::ShadowScale);

    // The cube goes, but the transform convention does not: PawnHeight still
    // places the transform and every distance check in the game reads it, so
    // switching character cannot move the player.
    SetMesh(nullptr);

    // Nothing of the previous character survives - no lingering shield, no
    // stale immunity, no half-finished stride.
    ClearAbilityState();

    PieceType ability = PieceType::Bishop;

    if (GetCharacterAbility(newCharacter, ability))
        CollectPiece(ability);

    UpdateShadow();
    UpdateAnimation(0.0f);
}

PieceType Pawn::GetCharacter() const
{
    return character;
}

const std::vector<std::shared_ptr<SceneNode>>& Pawn::GetRigParts() const
{
    return rig ? rig->GetParts() : NoParts();
}

bool Pawn::IsGrounded() const
{
    // Straight off the jump state, which is the authority on this.
    //
    // Before the jump system landed this had to infer being airborne from
    // the pawn's height above its lane, because nothing in the project
    // applied gravity or vertical velocity. Now that something does, the
    // animation reads that rather than re-deriving it: two answers to "is
    // the pawn on the ground" is one more than there should be, and the
    // inferred one needed a tolerance that would have delayed the jump pose
    // until the pawn was already a couple of centimetres up.
    return !isAirborne;
}

void Pawn::UpdateAnimation(float deltaTime)
{
    if (!rig || !animator)
        return;

    animator->Update(
        deltaTime,
        IsMoving(),
        IsGrounded(),
        GetSpeedMultiplier(),
        GetJumpVerticalVelocity());

    // The rig's origin is at its feet while the authoritative gameplay
    // transform is at the centre of the pawn's collision volume. Derive the
    // complete visual root from that transform so its X/Z movement, terrain
    // height and physical jump all travel through the same path. Using the
    // lane's fixed groundHeight here would discard the jump component and
    // leave only the camera (which follows the gameplay transform) moving.
    const glm::vec3& position = transform.GetPosition();

    rig->SetGroundPosition(
        position - glm::vec3(0.0f, GameConfig::PawnHeight * 0.5f, 0.0f));

    rig->SetHeadingDegrees(transform.GetRotation().y);

    rig->ApplyPose(animator->GetPose());
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

float Pawn::GetJumpVerticalVelocity() const
{
    return jumpVerticalVelocity;
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

bool Pawn::IsMovementInputActive() const
{
    return movementInputActive;
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