#pragma once

#include <glm.hpp>
#include <memory> // <-- Add this

#include "ChessPiece.h"
#include "GroundShadow.h"
#include "WorldObject.h"
#include "PieceMeshFactory.h" // <-- Add this

class Level;

/// The player's chess pawn.
///
/// GDD section 2: free top-down movement (not tile-hopping), clamped to the
/// playable width, following the terrain height of whatever lane the pawn is
/// currently over. Collision response (blocking, damage, knockback) is
/// deliberately NOT handled here -- Ayub owns how the pawn moves, Kaung owns
/// what happens when it collides with something. This class only exposes
/// the movement state (velocity, current row) those other systems need.
///
/// GDD section 3: also owns the collectible chess-piece ability loop --
/// banking a collected ally, activating it on Space, and the resulting
/// speed/immunity/shield effects. As with movement, this class only
/// exposes the resulting state (IsImmuneToHazards, HasShield, ...); it is
/// Kaung's collision response that should actually read and act on it.
class Pawn : public WorldObject
{
public:

    Pawn();

    /// Builds the placeholder mesh and applies the spawn pose.
    void Initialize() override;

    /// Reads input, moves freely along X/Z, clamps to the playable width,
    /// follows the terrain height of the pawn's current lane, and advances
    /// any active chess-piece ability.
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

    /// Row of the lane the pawn currently stands over, derived from its
    /// world Z. Hazard spawning and collision can key off this to know
    /// which lane's rules currently apply.
    int GetCurrentRow() const;

    /// The pawn owns its shadow and keeps it underneath itself.
    const WorldObject& GetShadow() const;

    //-----------------------------------------------------------
    // Chess-piece abilities (GDD section 3)
    //-----------------------------------------------------------

    /// Called when the pawn touches a collectible ally on the field.
    /// Only Bishop/Knight/Rook/Queen are meaningful here; anything else
    /// is ignored. Replaces whatever was previously banked (only one
    /// stored ability at a time, per the GDD's "Space: activate stored
    /// chess ability, if available").
    void CollectPiece(PieceType type);

    bool HasStoredPiece() const;

    /// Only meaningful when HasStoredPiece() is true.
    PieceType GetStoredPieceType() const;

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
    /// pawn toward its movement direction and checks for an ability
    /// activation key press.
    void HandleInput();

    /// Snaps the pawn's Y to the surface height of whichever lane its
    /// current Z falls over, so raised/sunken lanes read as real steps.
    void ApplyTerrainHeight();

    /// Consumes the stored piece (if any) and starts its effect.
    void TryActivateAbility();

    /// Counts down an active timed ability and clears it on expiry.
    void UpdateAbility(float deltaTime);

    /// Puts the shadow back under the pawn's current position.
    void UpdateShadow();

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

    bool spaceKeyWasDown = false;

    // --- Ability state ---

    bool hasStoredPiece = false;

    PieceType storedPieceType = PieceType::Bishop;

    bool abilityActive = false;

    PieceType activeAbilityType = PieceType::Bishop;

    float abilityTimeRemaining = 0.0f;

    bool shieldAvailable = false;

    bool bishopPulsePending = false;

    float slowMultiplier = 1.0f;

    float slowTimer = 0.0f;

    float knockbackTimer = 0.0f;

    // --- NEW HP VARIABLE ---
    float health = 5.0f; // Pawn starts with 5 HP

    glm::vec3 pawnBaseScale;
};