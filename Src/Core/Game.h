#pragma once

#include <memory>
#include <vector>

#include "Camera3D.h"
#include "ChessPiece.h"
#include "CollectibleManager.h"
#include "HazardManager.h"
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

    /// Points the camera at the pawn. Called on start-up and every frame,
    /// so this already behaves as a follow camera: it simply has nothing
    /// to follow until the pawn can move.
    void UpdateCamera();

    /// TEMPORARY: spawns one example of each moving-hazard pattern so the
    /// system is visible before Liyuu's level data decides real placement.
    /// Delete this method and its call in Initialize once that lands,
    /// exactly like BuildShowcase above.
    void SpawnExampleHazards();

    /// TEMPORARY: same idea as SpawnExampleHazards, for the four
    /// collectible allies. Delete once Liyuu's level data places these.
    void SpawnExampleCollectibles();

    std::shared_ptr<Camera3D> camera;

    std::shared_ptr<MeshRenderer> renderer;

    std::shared_ptr<Level> level;

    std::shared_ptr<Pawn> pawn;

    /// Own one model per type. Held by the game so the meshes are released
    /// in Shutdown, while the GL context is still alive.
    std::shared_ptr<PieceMeshLibrary> pieceMeshes;

    std::shared_ptr<ObstacleMeshLibrary> obstacleMeshes;

    std::shared_ptr<HazardManager> hazardManager;

    std::shared_ptr<CollectibleManager> collectibleManager;

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
