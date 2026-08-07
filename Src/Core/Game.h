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

    /// How many of the level's checkpoint gates have been reached and
    /// started opening at least once, 0..GetCheckpointGates().size().
    /// Drives the HUD's checkpoint pip row.
    int GetActivatedCheckpointCount() const;

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

    /// Registers one lane's repeating fireball launches.
    ///
    /// The fireball is lobbed in from beyond the edge of the field and comes
    /// down inside the lane it was fired along: laneZ is used for both ends
    /// of the flight, so the arc rises and falls without ever leaving the
    /// row. fromLeft only picks which edge it enters from.
    ///
    /// Every number involved - speed, arc height, travel distance, spawn
    /// margin, cooldown - lives in GameConfig, so retuning the hazard does
    /// not mean editing level layout code.
    void RegisterFireballVolley(
        float surfaceHeight,
        float laneZ,
        bool fromLeft);

    //---------------------------------------------------------
    // Level 1 hazard lanes
    //
    // One named call per lane, so BuildLevelHazards reads as a list of what
    // threatens where rather than as repeated spawn plumbing, and so the
    // three projectile types stay visibly different from each other at the
    // call site.
    //---------------------------------------------------------

    /// An arrow sweeping the full width of a row, on a loop.
    ///
    /// Straight, edge to edge, and never stops - the readable baseline the
    /// other two are varied against.
    void RegisterArrowLane(
        float surfaceHeight,
        float laneZ,
        bool fromLeft,
        float speed);

    /// A spear thrown in on an arc, landing inside the row.
    ///
    /// Deliberately not an arrow: it curves, it comes down at a chosen spot
    /// rather than crossing the whole map, and where it lands stays dangerous
    /// for a moment afterwards. landingOffset shifts the impact point along
    /// the row from the thrower's side, so two launchers on one row can
    /// cover different ground.
    void RegisterSpearVolley(
        float surfaceHeight,
        float laneZ,
        bool fromLeft,
        float interval,
        float initialDelay,
        float landingOffset);

    /// A cannonball fired across part of a row.
    ///
    /// Faster than an arrow and short of the far edge, per the GDD, so the
    /// far side of its row is a place to wait it out.
    void RegisterCannonballLane(
        float surfaceHeight,
        float laneZ,
        bool fromLeft,
        float interval,
        float initialDelay,
        float reach);

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

    /// Loads the individual PNG frames used by the lightning warning,
    /// strike and impact sprites.
    void LoadLightningSprites();

    /// Loads the individual PNG frames used by the fireball projectile and
    /// lingering impact sprites.
    void LoadFireballSprites();

    /// Adds per-frame lightning sprites for active lightning hazards.
    /// These are rebuilt every Render call from the hazard timing state.
    void AppendLightningSprites(std::vector<Sprite>& frameSprites) const;

    /// Adds per-frame fireball sprites for the flying projectile and the
    /// temporary impact zone left behind after it lands.
    void AppendFireballSprites(std::vector<Sprite>& frameSprites) const;

    /// Marks where a spear in flight is going to come down, and rings the
    /// broken ground it leaves once it has. The spear itself is a real mesh
    /// and is drawn by the 3D pass; these are only the ground markings that
    /// make it fair to dodge and obvious afterwards.
    void AppendSpearSprites(std::vector<Sprite>& frameSprites) const;

    //---------------------------------------------------------
    // Bishop / Queen clearing ability
    //
    // Gameplay and feedback are deliberately two steps: the clearing itself
    // is HazardManager::ClearStationaryObstacles, which knows nothing about
    // drawing, and everything below only ever reads the positions it
    // reported back.
    //---------------------------------------------------------

    /// Runs the Bishop's (and the Queen's) clearing pulse: removes the
    /// eligible stationary props around the pawn and starts the effect that
    /// shows which ones went.
    void TriggerAbilityClearPulse();

    /// Ages every live pulse and drops the finished ones.
    void UpdateAbilityPulses(float deltaTime);

    /// Adds the expanding ring and the per-obstacle bursts for any pulse
    /// still running. Rebuilt every Render from the timer, like the hazard
    /// VFX above.
    void AppendAbilityPulseSprites(std::vector<Sprite>& frameSprites) const;

    //---------------------------------------------------------
    // HUD
    //
    // Icon/pip/bar based -- the engine has no text rendering. Sprites are
    // rebuilt every frame from Pawn/checkpoint state, the same pattern as
    // the VFX passes above, and drawn last so they land on top.
    //---------------------------------------------------------

    /// Loads the placeholder HUD textures: the pip/bar squares, the
    /// universal ability icon, the interact prompt, and one silhouette
    /// icon per PieceType. Called once from Initialize.
    void LoadHudSprites();

    /// Adds this frame's HUD: health pips, checkpoint pips, the ability
    /// icon and its duration bar, the current piece icon, and the interact
    /// prompt.
    void AppendHudSprites(std::vector<Sprite>& frameSprites) const;

    /// Texture name registered for one piece's HUD silhouette icon. Used by
    /// the current-piece indicator.
    static const std::string& PieceIconTextureName(PieceType type);

    /// True while pawnPosition is within GameConfig::InteractRadius of a
    /// checkpoint gate or the King's Cage -- read-only mirror of the
    /// distance checks TryInteractWithCheckpointGate/TryInteractWithKingsCage
    /// make, so the HUD's interact prompt matches what E actually does
    /// without duplicating or triggering either one.
    bool IsNearInteractable(const glm::vec3& pawnPosition) const;

    //---------------------------------------------------------
    // Game over
    //---------------------------------------------------------

    /// Counts a death every time Pawn::ConsumeDeathPulse() reports one.
    /// GameConfig::MaxDeathsBeforeGameOver deaths trigger a Game Over:
    /// checkpoint progress clears, the pawn is sent back to the level's
    /// initial spawn point, and the banner timer starts. Also ages that
    /// timer down every frame. Called once per Update.
    void UpdateGameOver(float deltaTime);

    //---------------------------------------------------------
    // Victory
    //---------------------------------------------------------

    /// Once (and only once) per run, checks whether the pawn has reached
    /// the King inside the cage and, if so, latches hasWon and starts the
    /// victory banner timer. Also ages that timer down every frame. Called
    /// once per Update.
    void UpdateVictory(float deltaTime);

    /// Ages every dying prop, shrinking and fading it, and drops the ones
    /// whose reaction has finished.
    void UpdateDyingObstacles(float deltaTime);

    /// One in-flight Bishop/Queen pulse.
    ///
    /// Holds where the ability fired and where each prop it destroyed used
    /// to stand, so the effect can mark both. Positions are copied rather
    /// than pointed at precisely because the obstacles they came from no
    /// longer exist by the time this is drawn.
    struct AbilityPulse
    {
        glm::vec3 origin = glm::vec3(0.0f);

        std::vector<glm::vec3> clearedPositions;

        float elapsed = 0.0f;
    };

    /// A prop or sheep the ability has destroyed, still playing out its
    /// reaction.
    ///
    /// It has already been taken out of the collision and gameplay lists -
    /// this list exists only so something visible remains to animate. The
    /// starting ground position and scale are captured because the shrink
    /// and the sink are both measured from them.
    struct DyingObstacle
    {
        std::shared_ptr<GroundEntity> visual;

        glm::vec3 startGroundPosition = glm::vec3(0.0f);

        glm::vec3 startScale = glm::vec3(1.0f);

        float elapsed = 0.0f;
    };

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

    /// Parallel to checkpointGates: true once that gate has been reached
    /// and started opening at least once. Unlike checkpointGateOpening
    /// this is sticky -- set true and never cleared -- so it's what the
    /// HUD's checkpoint pip row reads.
    std::vector<bool> checkpointGateActivated;

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

    /// Cached window size in pixels, kept up to date by SetViewportSize, so
    /// screen-space HUD math (right/centre-anchored elements) doesn't need
    /// a separate size parameter threaded through Update/Render.
    float hudScreenWidth = 1280.0f;

    float hudScreenHeight = 720.0f;

    /// Deaths since the last Game Over (or level start). Reset to 0 the
    /// instant it hits GameConfig::MaxDeathsBeforeGameOver.
    int deathCount = 0;

    /// Counts down from GameConfig::GameOverBannerDuration once a Game
    /// Over fires. The reset itself already happened; this only times how
    /// long the banner stays on screen.
    float gameOverBannerTimer = 0.0f;

    /// True the instant the pawn reaches the King. Latched, not reset --
    /// unlike Game Over, Victory is a terminal, one-shot event.
    bool hasWon = false;

    /// Counts down from GameConfig::VictoryBannerDuration once Victory
    /// fires.
    float victoryBannerTimer = 0.0f;

    /// Counts down from GameConfig::ControlsScreenDuration, starting the
    /// instant Initialize sets it -- no trigger condition, it just plays
    /// out once at the start of every run.
    float controlsScreenTimer = 0.0f;

    /// Resolves moving hazards and the real stationary level props against
    /// the pawn every frame.
    std::unique_ptr<HazardCollision> hazardCollision;

    /// Stationary props placed on the SpikeMud and FenceTree lanes. The same
    /// instances are rendered and supplied to HazardCollision.
    std::vector<std::shared_ptr<StaticObstacle>> stationaryHazards;

    /// Bishop/Queen clearing pulses currently playing. More than one can be
    /// live at a time if the ability is used again before the last finished.
    std::vector<AbilityPulse> abilityPulses;

    /// Props and sheep mid-destruction. Drawn, but no longer part of
    /// collision or gameplay.
    std::vector<DyingObstacle> dyingObstacles;
};
