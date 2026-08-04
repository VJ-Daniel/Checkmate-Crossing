#pragma once

#include <memory>
#include <vector>

#include "CageMeshFactory.h"
#include "Camera3D.h"
#include "Cheat.h"
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
#include "HazardCollision.h"

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

    /// The first gate standing on a checkpoint lane, or null when the level
    /// has none. Kept for callers that only ever cared about one gate; see
    /// GetCheckpointGates() for the rest.
    CheckpointGate* GetCheckpointGate();

    /// Every checkpoint gate standing on the level, in level order.
    const std::vector<std::shared_ptr<CheckpointGate>>& GetCheckpointGates() const;

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

    /// Stands one checkpoint gate on every one of the level's checkpoint
    /// lanes.
    ///
    /// Unlike the showcase this is a real placement, not scaffolding: the
    /// GDD puts a checkpoint roughly every 20 units of progress, and a gate
    /// is what marks each one. The first is past the opening view, so it
    /// comes into sight once the pawn can walk towards it.
    void BuildCheckpointGate();

    /// Draws one chess piece, whether it is a single baked mesh or an
    /// animated rig made of several.
    void DrawPiece(const ChessPiece& piece);

    void DrawPawn();

    /// Places the visual-only cage and captive King in the final goal area.
    /// Gameplay will later decide when its separate door should open.
    void BuildKingsCage();

    /// Applies the reference game's camera setup: an orthographic camera
    /// tilted down over the battlefield, at the angle and zoom recorded in
    /// GameConfig.
    void ConfigureCamera();

    /// Advances the fixed-angle camera only when the pawn crosses its X/Z
    /// dead zone, then clamps the visible ground footprint to the finished
    /// boundary envelope. Jump height is intentionally visual, not camera
    /// motion, so a jump inside the lock space leaves the camera still.
    void UpdateCamera();

    /// Places every stationary prop the level's SpikeMud and FenceTree
    /// sections need, reading real lane rows from `level`.
    void BuildLevelObstacles();

    /// Places every moving hazard's real lane assignment, following the
    /// GDD's escalation grouping (arrows/spears together, cannonballs/
    /// rolling rocks together, fireballs/lightning/rolling logs together).
    void BuildLevelHazards();

    /// Places the four collectible allies just ahead of the section each
    /// ability is most useful for.
    void BuildLevelCollectibles();

    //---------------------------------------------------------
    // Interaction (E)
    //
    // Pawn only reports "E was pressed" (ConsumeInteractPulse) since it
    // has no reference to the checkpoint gate or king's cage door. This is
    // where that press gets resolved against them, falling back to the
    // pawn's banked ability if neither is in range.
    //---------------------------------------------------------

    /// If the pawn is within GameConfig::InteractRadius of any checkpoint
    /// gate, starts it opening (if closed), banks it as the pawn's most
    /// recently activated checkpoint (so a later respawn returns there
    /// instead of the level's original start), and returns true either way
    /// -- standing at a gate means E belongs to it, not the ability.
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

    /// Distinguishes the first exact placement from later dead-zone updates.
    bool cameraFollowInitialized = false;

    std::shared_ptr<MeshRenderer> renderer;

    std::shared_ptr<Level> level;

    std::shared_ptr<Pawn> pawn;

    /// Own one model per type. Held by the game so the meshes are released
    /// in Shutdown, while the GL context is still alive.
    std::shared_ptr<PieceMeshLibrary> pieceMeshes;

    /// Developer-only character switching. Inert unless
    /// CheatConfig::EnableCheatKeys is on.
    CheatSystem cheats;

    std::shared_ptr<ObstacleMeshLibrary> obstacleMeshes;

    std::shared_ptr<GateMeshLibrary> gateMeshes;

    std::shared_ptr<CageMeshLibrary> cageMeshes;

    /// Every checkpoint entrance standing on the level, one per checkpoint
    /// lane, in level order. Their parts share the library's meshes, so
    /// each extra gate costs a handful of transforms and nothing else.
    std::vector<std::shared_ptr<CheckpointGate>> checkpointGates;

    /// Reused between frames so rebuilding the gate's solid volumes does not
    /// allocate every tick.
    std::vector<CollisionBox> gateCollisionBoxes;

    /// Final visual objective. The King remains a normal ChessPiece rather
    /// than being baked into the cage, and both door leaves are separate
    /// meshes owned by KingsCage so they rotate around their outer hinges.
    std::shared_ptr<KingsCage> kingsCage;

    std::shared_ptr<ChessPiece> capturedKing;

    std::shared_ptr<HazardManager> hazardManager;

    std::shared_ptr<CollectibleManager> collectibleManager;

    /// Parallel to checkpointGates: true while E has started that gate
    /// opening and it hasn't reached CheckpointGate::GetMaxDoorAngle() yet.
    std::vector<bool> checkpointGateOpening;

    /// Same idea for the king's cage doors. Both share one opening-angle
    /// magnitude with opposite rotation signs, tracked here rather than on
    /// either individual leaf.
    bool kingsCageDoorOpening = false;

    float kingsCageDoorAngle = 0.0f;

    /// Shared quad and shader for every sprite. Held here so its GL objects
    /// are released in Shutdown, while the context is still alive.
    std::shared_ptr<SpriteRenderer> spriteRenderer;

    /// Sprites drawn after the opaque world, every frame. Empty until sprite
    /// assets exist.
    std::vector<Sprite> sprites;

    /// Resolves moving hazards and the real stationary level props against
    /// the pawn every frame.
    std::unique_ptr<HazardCollision> hazardCollision;

    /// Stationary props placed on the SpikeMud and FenceTree lanes. The same
    /// instances are rendered and supplied to HazardCollision.
    std::vector<std::shared_ptr<StaticObstacle>> stationaryHazards;
};
