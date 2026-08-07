#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>
#include <memory> // <-- Add this

#include "ChessPiece.h"
#include "GroundShadow.h"
#include "WorldObject.h"
#include "PieceMeshFactory.h" // <-- Add this

class Level;
class PieceRig;
class PieceAnimator;
class PieceMeshLibrary;
class SceneNode;

/// The player's chess pawn.
///
/// GDD section 2: free top-down movement (not tile-hopping), clamped to the
/// complete playable rectangle, following the ground height of whatever lane
/// the pawn is currently over, plus a jump for clearing Fence/Rock/Palisade
/// obstacles.
/// Collision response (blocking, damage, knockback) is deliberately NOT
/// handled here -- Ayub owns how the pawn moves, Kaung owns what happens
/// when it collides with something. This class only exposes the movement
/// state (velocity, current row, IsAirborne) those other systems need.
///
/// GDD section 3: also owns the collectible chess-piece ability loop --
/// banking a collected ally and the resulting speed/immunity/shield
/// effects. As with movement, this class only exposes the resulting state
/// (IsImmuneToHazards, HasShield, ...); it is Kaung's collision response
/// that should actually read and act on it.
///
/// Controls: Space jumps, E interacts. E is context-sensitive and not
/// fully resolved here -- ConsumeInteractPulse() reports "E was pressed"
/// for whoever owns both the pawn and the interactable world (currently
/// Game) to resolve against nearby doors/gates; TryActivateAbility() is
/// the fallback that same code should call when nothing else was in range.
class Pawn : public WorldObject
{
public:

    Pawn();

    /// Out of line, for the forward-declared animation members.
    ~Pawn() override;

    /// Builds the placeholder mesh and applies the spawn pose.
    void Initialize() override;

    //-----------------------------------------------------------
    // Visuals
    //
    // The pawn's body is a rig rather than the placeholder cube, but the
    // transform, the width and the height it moves and collides with are
    // unchanged -- the rig is hung off the transform, never the other way
    // round. Nothing about movement, terrain following or pickup radius
    // reads the model.
    //-----------------------------------------------------------

    /// Gives the pawn its animated model. Optional: without it the pawn
    /// falls back to the placeholder cube and everything else still works.
    void AttachRig(PieceMeshLibrary& meshLibrary);

    //-----------------------------------------------------------
    // Which character the player is
    //
    // The player is always this one Pawn object -- position, level
    // reference, checkpoint and camera all belong to it and survive a
    // change of character. Only the body and the banked ability change.
    //-----------------------------------------------------------

    /// Becomes another chess character in place.
    ///
    /// Swaps the model, the animation body plan, the shadow and the
    /// footprint used to resolve the playable bounds, then clears any
    /// ability the previous character had running and banks whichever one
    /// the new character is entitled to.
    ///
    /// Deliberately does not touch position, velocity, spawn point or the
    /// level reference: switching character is not respawning.
    ///
    /// Knight is rejected. It is the riderless horse, which is a collectible
    /// ally rather than something the player becomes -- MountedPawn is the
    /// playable mounted character.
    void SetCharacter(PieceType character, PieceMeshLibrary& meshLibrary);

    PieceType GetCharacter() const;

    /// Which ability a character carries, or false when it has none.
    ///
    /// The one place the mapping lives. Pawn walks with no ability at all,
    /// the King has none implemented yet, and the mounted pawn inherits the
    /// horse's speed and hazard immunity because that is what riding one
    /// does -- it is not the standalone Knight's own ability slot.
    static bool GetCharacterAbility(
        PieceType character,
        PieceType& abilityType);

    /// Drops every ability effect and anything banked but unused.
    ///
    /// Called on a character switch so a Rook shield or a Knight's immunity
    /// cannot survive onto a body that should not have it.
    void ClearAbilityState();

    /// The parts to draw, or empty when the pawn is still a cube.
    const std::vector<std::shared_ptr<SceneNode>>& GetRigParts() const;

    /// True while the pawn is resting on the ground it stands over.
    ///
    /// The inverse of IsAirborne, and what the animation reads to decide
    /// between the walk and the jump pose. Kept as its own name because the
    /// animator is written in terms of being grounded, and because reading
    /// the jump flag directly from the animation would tie the two together
    /// more tightly than either wants.
    bool IsGrounded() const;

    /// Reads input, moves freely along X/Z, resolves the pawn's footprint
    /// against all four playable edges, follows the terrain height of the
    /// current lane, applies jump gravity, and advances any active ability.
    void Update(float deltaTime) override;

    /// Sets the point on the ground the pawn stands on. The pawn's own
    /// height is added automatically, so callers work in ground coordinates.
    void SetSpawnPosition(const glm::vec3& groundPosition);

    const glm::vec3& GetSpawnPosition() const;

    /// Returns the pawn to its spawn point. Checkpoints will reuse this by
    /// calling SetSpawnPosition first.
    void Respawn();

    /// Non-owning reference used to look up ground height as the pawn moves
    /// freely across lanes. Call once, after both the level and the pawn
    /// have been constructed (see Game::Initialize).
    void SetLevel(const Level* level);

    // --- NEW FUNCTION ---
    void SetMeshLibrary(std::shared_ptr<PieceMeshLibrary> library);
    // --------------------

    float GetMoveSpeed() const;

    void SetMoveSpeed(float unitsPerSecond);

    /// Current world-space X/Z velocity, already including any active
    /// speed-boost ability. Read by collision response (Kaung) and
    /// animation blending (John); this class does not clear or react to
    /// collisions itself.
    const glm::vec3& GetVelocity() const;

    bool IsMoving() const;

    /// True while a movement key is held, whether or not the pawn actually
    /// moved this frame.
    ///
    /// IsMoving reads velocity, which collision legitimately zeroes when the
    /// pawn is pressed against a wall - so driving the walk cycle from it
    /// froze the character mid-stride the moment it touched anything. The
    /// animation asks what the player is trying to do; the gameplay still
    /// asks what actually happened.
    bool IsMovementInputActive() const;

    /// Row of the lane the pawn currently stands over, derived from its
    /// world Z. Hazard spawning and collision can key off this to know
    /// which lane's rules currently apply.
    int GetCurrentRow() const;

    /// The pawn owns its shadow and keeps it underneath itself.
    const WorldObject& GetShadow() const;

    //-----------------------------------------------------------
    // Jumping (GDD section 4: Fence/Rock/Palisade "can be jumped over")
    //-----------------------------------------------------------

    /// True from the moment Space leaves the ground until the pawn lands.
    /// Kaung's collision response should read this to decide whether a
    /// Fence/Rock/Palisade blocks the pawn or is being jumped over (and,
    /// for Palisade specifically, whether the "jumping over it damages the
    /// player" case applies).
    bool IsAirborne() const;

    /// Height currently added on top of the terrain by the jump, in world
    /// units. Zero while grounded. Purely a movement/visual number -- not
    /// a substitute for whatever height threshold Kaung's collision uses
    /// to decide a jump cleared something.
    float GetJumpHeight() const;

    /// Vertical speed of the jump: positive climbing, negative falling,
    /// zero on the ground. Read by the animation to tell pushing off from
    /// hanging in the air; it changes nothing about the jump itself.
    float GetJumpVerticalVelocity() const;

    //-----------------------------------------------------------
    // Interaction (E)
    //-----------------------------------------------------------

    /// True exactly once, on the frame E is pressed, then clears itself.
    /// Interaction targets (doors, gates, switches, ...) are part of the
    /// wider level, which this class deliberately has no reference to --
    /// whoever owns both the pawn and those objects (currently Game)
    /// should check this every frame, resolve it against anything nearby,
    /// and call TryActivateAbility() itself if nothing was in range.
    bool ConsumeInteractPulse();

    //-----------------------------------------------------------
    // Chess-piece abilities (GDD section 3)
    //-----------------------------------------------------------

    /// Called when the pawn touches a collectible ally on the field.
    /// Only Bishop/Knight/Rook/Queen are meaningful here; anything else
    /// is ignored. Replaces whatever was previously banked (only one
    /// stored ability at a time).
    void CollectPiece(PieceType type);

    bool HasStoredPiece() const;

    /// Only meaningful when HasStoredPiece() is true.
    PieceType GetStoredPieceType() const;

    /// Consumes the stored piece (if any) and starts its effect. Public
    /// because it's no longer fired directly off a key press -- see the
    /// class comment on ConsumeInteractPulse() for who calls this and when.
    void TryActivateAbility();

    /// True while a timed ability (Knight/Rook/Queen) is currently in
    /// effect. Bishop has no ongoing state -- see
    /// ConsumeBishopActivationPulse() instead.
    bool IsAbilityActive() const;

    /// Only meaningful when IsAbilityActive() is true.
    PieceType GetActiveAbilityType() const;

    /// Multiplier currently applied to the pawn's base move speed: 1.0
    /// normally, boosted while Knight or Queen is active. John's animation
    /// blending can also read this to speed up a run cycle, for instance.
    float GetSpeedMultiplier() const;

    /// True while hazard collisions should be ignored entirely (Knight or
    /// Queen). Kaung's collision response should check this before
    /// applying any hazard effect.
    bool IsImmuneToHazards() const;

    /// True while a shield charge is available (Rook or Queen). Kaung's
    /// collision response should call ConsumeShield() instead of applying
    /// damage when this is true.
    bool HasShield() const;

    /// Spends the shield charge, blocking whatever hit triggered it.
    void ConsumeShield();

    /// True exactly once, on the frame Bishop's ability activates, then
    /// clears itself. Bishop removes nearby obstacles/hazards, which
    /// means mutating the world -- this class deliberately does not hold
    /// a reference to HazardManager to do that itself; whoever owns both
    /// (currently Game) should check this every frame and act on it.
    bool ConsumeBishopActivationPulse();

    /// Set the pawn's velocity directly (for knockback from collisions)
    void SetVelocity(const glm::vec3& newVelocity) { velocity = newVelocity; }

    /// Apply a slow effect to the pawn
    void ApplySlow(float duration, float amount);

    /// Get the slow multiplier (0.0 = fully slowed, 1.0 = normal speed)
    float GetSlowMultiplier() const { return slowMultiplier; }

    /// Check if the pawn is currently slowed
    bool IsSlowed() const { return slowMultiplier < 1.0f; }

    // --- NEW HP FUNCTIONS ---
    /// Apply damage to the pawn. If health reaches 0, it respawns.
    void TakeDamage(float damage);

    /// Get the current health of the pawn.
    float GetHealth() const { return health; }

    void SetKnockback(const glm::vec3& knockbackVector, bool isCow /* = false */);

    bool IsKnockedBackByCow() const { return knockedBackByCow; }

    float GetKnockbackTimer() const { return knockbackTimer; }

private:
    bool knockedBackByCow = false;
    /// Turns WASD/arrow input into a normalized move direction and applies
    /// it as velocity (scaled by any active speed ability); also faces the
    /// pawn toward its movement direction and checks for a jump or an
    /// interact key press.
    void HandleInput();

    /// Snaps the pawn's Y to the surface height of whichever lane its
    /// current Z falls over, plus the current jump offset, so raised/
    /// sunken lanes read as real steps and a jump lifts cleanly off them.
    void ApplyTerrainHeight();

    /// Starts a jump if the pawn is currently grounded; otherwise a no-op.
    void TryJump();

    /// Applies gravity to an in-progress jump and lands it at zero height.
    void UpdateJump(float deltaTime);

    /// Counts down an active timed ability and clears it on expiry.
    void UpdateAbility(float deltaTime);

    /// Swaps only the visible body and collision footprint. Ability state is
    /// intentionally left alone so temporary ability forms can expire cleanly.
    bool SetVisualCharacter(PieceType newCharacter, PieceMeshLibrary& meshLibrary);

    /// Applies the model associated with a just-activated ability.
    void ApplyAbilityVisual(PieceType abilityType);

    /// Returns the player to the normal pawn model after a temporary form.
    void RestorePawnVisual();

    /// Puts the shadow back under the pawn's current position.
    void UpdateShadow();

    /// Advances the walk/jump animation and stands the rig on the pawn.
    ///
    /// Deliberately the last thing Update does and deliberately reads only
    /// values movement has already finished writing, so animation can never
    /// feed back into where the pawn is.
    void UpdateAnimation(float deltaTime);

    std::unique_ptr<PieceRig> rig;

    std::unique_ptr<PieceAnimator> animator;

    /// Which chess character the player currently wears.
    PieceType character = PieceType::Pawn;

    /// Half the character's width, used to resolve the playable bounds.
    ///
    /// A member rather than GameConfig::PawnWidth directly, because the
    /// characters are not all the same size and the bounds have to follow
    /// whichever body the player is currently in.
    float footprintRadius = 0.0f;

    glm::vec3 spawnPosition;

    /// Y of the ground the pawn is standing on, which the shadow sits on.
    float groundHeight;

    GroundShadow shadow;

    const Level* level = nullptr;

    // --- NEW MEMBER VARIABLE ---
    std::shared_ptr<PieceMeshLibrary> meshLibrary; // To create piece meshes
    // --------------------------

    float moveSpeed = 4.0f;

    glm::vec3 velocity = glm::vec3(0.0f);

    // --- Jump state ---

    bool spaceKeyWasDown = false;

    bool isAirborne = false;

    float jumpHeight = 0.0f;

    float jumpVerticalVelocity = 0.0f;

    // --- Interact state ---

    bool interactKeyWasDown = false;

    bool interactPulsePending = false;

    // --- Ability state ---

    bool hasStoredPiece = false;

    PieceType storedPieceType = PieceType::Bishop;

    bool abilityActive = false;

    PieceType activeAbilityType = PieceType::Bishop;

    float abilityTimeRemaining = -1.0f;

    float abilityVisualTimeRemaining = 0.0f;

    bool shieldAvailable = false;

    bool bishopPulsePending = false;

    float slowMultiplier = 1.0f;

    float slowTimer = 0.0f;

    bool movementInputActive = false;

    float knockbackTimer = 0.0f;

    // --- NEW HP VARIABLE ---
    float health = 5.0f; // Pawn starts with 5 HP

    glm::vec3 pawnBaseScale;
};
