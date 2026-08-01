#pragma once

#include <memory>
#include <vector>

#include "CageMeshFactory.h"
#include "Camera3D.h"
#include "CheckpointGate.h"
#include "ChessPiece.h"
#include "CollectibleManager.h"
#include "GateMeshFactory.h"
#include "HazardManager.h"
#include "KingsCage.h"
#include "Level.h"
#include "MeshRenderer.h"
#include "Obstacle.h"
#include "ObstacleMeshFactory.h"
#include "Pawn.h"
#include "PieceMeshFactory.h"
#include "Sprite.h"
#include "SpriteRenderer.h"

/// Owns the high-level lifetime of one Checkmate Crossing session: the
/// camera, the world renderer, the battlefield and the player's pawn.
///
/// This is the foundation stage, so there is no game state machine yet.
/// When one is needed it belongs here, wrapping Update and Render.
class Game
{
public:

    Game();

    ~Game();

    /// Creates the rendering services, builds the level and spawns the pawn.
    bool Initialize(
        float screenWidth,
        float screenHeight);

    /// Advances the world using frame-independent elapsed time.
    void Update(float deltaTime);

    /// Draws the battlefield first and the pawn on top of it.
    void Render();

    /// Releases OpenGL, world and resource objects safely.
    void Shutdown();

    Camera3D& GetCamera();

    MeshRenderer& GetRenderer();

    Level& GetLevel();

    Pawn& GetPawn();

    PieceMeshLibrary& GetPieceMeshes();

    ObstacleMeshLibrary& GetObstacleMeshes();

    /// The gate standing on the checkpoint lane, or null when the level has
    /// no checkpoint in it.
    CheckpointGate* GetCheckpointGate();

    /// Visual-only final objective and the King standing inside it.
    /// Both are null when the level has no King's Cage area.
    KingsCage* GetKingsCage();

    ChessPiece* GetCapturedKing();

    /// Every active moving hazard (arrows, spears, cannonballs, fireballs,
    /// rolling rocks/logs, the cow). Kaung's collision system should read
    /// this every frame; nothing here reacts to the player itself.
    HazardManager& GetHazardManager();

    /// Every collectible ally waiting to be picked up. Kaung's collision
    /// system should call Pawn::CollectPiece(...) when the pawn touches
    /// one; see the note on CollectibleManager::TryCollect for the
    /// placeholder currently standing in for that.
    CollectibleManager& GetCollectibleManager();

    SpriteRenderer& GetSpriteRenderer();

    //---------------------------------------------------------
    // Sprites
    //
    // The sprite system is wired up and idle: there are no sprite assets in
    // the project yet, so the list below is empty and the sprite pass costs
    // nothing. Registering one sprite is all it takes to start using it.
    //---------------------------------------------------------

    /// Queues a sprite to be drawn every frame until it is removed. Returns
    /// its index, which is what RemoveSprite and GetSprite take.
    std::size_t AddSprite(const Sprite& sprite);

    /// Mutable access, so an owner can move or re-frame its sprite - for
    /// instance driving it from a SpriteAnimation each Update.
    Sprite* GetSprite(std::size_t index);

    void RemoveSprite(std::size_t index);

    void ClearSprites();

    std::size_t GetSpriteCount() const;

    /// Keeps screen-space sprites correctly scaled after a window resize.
    void SetViewportSize(float width, float height);

private:

    /// TEMPORARY: lines the placeholder models up in front of the pawn so
    /// they can be inspected side by side - chess pieces, then stationary
    /// props, then hazards, one row each.
    ///
    /// Delete this method and its call in Initialize once the level places
    /// these for real. Nothing else depends on it.
    void BuildShowcase();

    /// Stands one checkpoint gate on the level's checkpoint lane.
    ///
    /// Unlike the showcase this is a real placement, not scaffolding: the
    /// GDD puts a checkpoint there and the gate is what marks it. It is
    /// past the opening view, so it comes into sight once the pawn can
    /// walk towards it.
    void BuildCheckpointGate();

    /// Draws one chess piece, whether it is a single baked mesh or an
    /// animated rig made of several.
    void DrawPiece(const ChessPiece& piece);

    void DrawPawn();

    /// Places the visual-only cage and captive King in the final goal area.
    /// Gameplay will later decide when its separate door should open.
    void BuildKingsCage();

    /// Fills one evenly spaced row of a showcase, centred on the board.
    /// Returns the ground position for the given slot.
    glm::vec3 GetShowcaseSlot(
        int lanesAhead,
        int slot,
        int slotCount,
        float spacing) const;

    /// Applies the reference game's camera setup: an orthographic camera
    /// tilted down over the battlefield, at the angle and zoom recorded in
    /// GameConfig.
    void ConfigureCamera();

    /// Points the camera at the pawn on start-up and every frame, producing
    /// the fixed-angle follow view while the pawn moves through the level.
    void UpdateCamera();

    /// TEMPORARY: spawns examples of the mesh-backed moving hazards so the
    /// system is visible before level data decides real placement. Fireball
    /// and Lightning stay unspawned until their deferred visuals exist.
    /// Delete this method and its call in Initialize once that lands,
    /// exactly like BuildShowcase above.
    void SpawnExampleHazards();

    /// TEMPORARY: same idea as SpawnExampleHazards, for the four
    /// collectible allies. Delete once Liyuu's level data places these.
    void SpawnExampleCollectibles();

    //---------------------------------------------------------
    // Interaction (E)
    //
    // Pawn only reports "E was pressed" (ConsumeInteractPulse) since it
    // has no reference to the checkpoint gate or king's cage door. This is
    // where that press gets resolved against them, falling back to the
    // pawn's banked ability if neither is in range.
    //---------------------------------------------------------

    /// If the pawn is within GameConfig::InteractRadius of the checkpoint
    /// gate, starts it opening (if closed) and returns true either way --
    /// standing at the gate means E belongs to it, not the ability.
    bool TryInteractWithCheckpointGate(const glm::vec3& pawnPosition);

    /// Same idea as TryInteractWithCheckpointGate, for the king's cage
    /// door.
    ///
    /// NOTE(Ayub): this only swings the door open. Whether that should also
    /// count as rescuing the king / winning the level is Kaung's call
    /// ("king rescue, win conditions" is his); nothing here decides that.
    bool TryInteractWithKingsCage(const glm::vec3& pawnPosition);

    /// Advances any door currently mid-open, a handful of degrees per
    /// second toward fully open. A placeholder tween -- John's animation
    /// pass owns "checkpoint activation" for real; this just makes E
    /// functionally open something instead of just changing a flag.
    void UpdateDoors(float deltaTime);

    std::shared_ptr<Camera3D> camera;

    std::shared_ptr<MeshRenderer> renderer;

    std::shared_ptr<Level> level;

    std::shared_ptr<Pawn> pawn;

    /// Own one model per type. Held by the game so the meshes are released
    /// in Shutdown, while the GL context is still alive.
    std::shared_ptr<PieceMeshLibrary> pieceMeshes;

    std::shared_ptr<ObstacleMeshLibrary> obstacleMeshes;

    std::shared_ptr<GateMeshLibrary> gateMeshes;

    std::shared_ptr<CageMeshLibrary> cageMeshes;

    /// The checkpoint entrance. Its parts share the library's meshes, so a
    /// second gate later on costs a handful of transforms and nothing else.
    std::shared_ptr<CheckpointGate> checkpointGate;

    /// Final visual objective. The King remains a normal ChessPiece rather
    /// than being baked into the cage, and the door is a separate mesh owned
    /// by KingsCage so future rescue behavior can rotate it around its hinge.
    std::shared_ptr<KingsCage> kingsCage;

    std::shared_ptr<ChessPiece> capturedKing;

    std::shared_ptr<HazardManager> hazardManager;

    std::shared_ptr<CollectibleManager> collectibleManager;

    /// True while E has started the checkpoint gate opening and it hasn't
    /// reached CheckpointGate::GetMaxDoorAngle() yet.
    bool checkpointGateOpening = false;

    /// Same idea for the king's cage door, which -- unlike the checkpoint
    /// gate -- has no angle getter of its own, so the angle is tracked
    /// here instead.
    bool kingsCageDoorOpening = false;

    float kingsCageDoorAngle = 0.0f;

    /// Shared quad and shader for every sprite. Held here so its GL objects
    /// are released in Shutdown, while the context is still alive.
    std::shared_ptr<SpriteRenderer> spriteRenderer;

    /// Sprites drawn after the opaque world, every frame. Empty until sprite
    /// assets exist.
    std::vector<Sprite> sprites;

    /// TEMPORARY: the showcase rows described above.
    std::vector<std::shared_ptr<ChessPiece>> showcasePieces;

    std::vector<std::shared_ptr<Obstacle>> showcaseObstacles;
};
