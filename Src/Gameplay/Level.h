#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "Lane.h"
#include "Mesh.h"

/// Axis-aligned horizontal world bounds. X is map width, Z is progression,
/// and Y is deliberately absent so movement boundaries remain jump-proof.
struct LevelBoundsXZ
{
    float minX = 0.0f;
    float maxX = 0.0f;
    float minZ = 0.0f;
    float maxZ = 0.0f;
};

/// Builds and owns the battlefield: the ordered strip of lanes that runs
/// from the start area to the king's cage, following the layout in GDD
/// section 4.
///
/// The level is intentionally empty for now. Obstacles, checkpoints and the
/// king's cage will be added as extra WorldObjects owned alongside the lanes,
/// each one anchored to the lane it belongs to.
class Level
{
public:

    Level();

    /// Creates every lane of the level in order. Safe to call again to
    /// rebuild the map from scratch.
    void Build();

    void Update(float deltaTime);

    const std::vector<std::shared_ptr<Lane>>& GetLanes() const;

    /// Scenery blocks forming the finished perimeter outside the walkable
    /// board.
    ///
    /// Purely visual: continuous player containment comes from
    /// ClampToPlayableBounds, while these varied blocks hide the world
    /// outside that logical boundary from the orthographic camera.
    const std::vector<std::shared_ptr<WorldObject>>& GetDecorations() const;

    /// Where the pawn starts, on the surface of the last start-area row.
    glm::vec3 GetPlayerSpawnPosition() const;

    /// Row index the pawn starts on.
    int GetSpawnRow() const;

    /// Lane occupying a row, or nullptr when the row is off the level.
    /// Anything that needs to know how high the ground is at a given row -
    /// the pawn once it moves, obstacles, the king's cage - goes through
    /// this rather than assuming a flat board.
    const Lane* GetLane(int row) const;

    /// First row occupied by a lane of the given type, or -1 when the level
    /// has none.
    ///
    /// Anything that belongs at a named point of the level - the checkpoint
    /// gate or the king's cage - asks for its row this way instead
    /// of counting the layout by hand and going stale the moment a section
    /// is made longer.
    int FindRowOfType(LaneType type) const;

    /// Logical edges of the playable road, before an entity's own footprint
    /// is inset. Derived from the lane list rather than a hard-coded row
    /// count, so extending the level automatically extends its end boundary.
    LevelBoundsXZ GetPlayableBounds() const;

    /// Outer envelope occupied by the dense decorative perimeter. Camera
    /// clamping uses this rather than the smaller player bounds because an
    /// orthographic viewport displays several world units around its target.
    LevelBoundsXZ GetBoundaryVisualBounds() const;

    /// Resolves an object's centre inside the playable rectangle after
    /// accounting for its horizontal footprint. This is the project's
    /// continuous four-edge boundary constraint: it has no corner gaps and
    /// ignores Y, so jumping cannot cross it.
    glm::vec3 ClampToPlayableBounds(
        const glm::vec3& position,
        float halfWidth,
        float halfDepth) const;

    /// How far from the centre the pawn may walk.
    static float GetPlayableHalfWidth();

    /// Converts a row index into its world-space Z.
    /// Row 0 sits at the origin and the level runs toward -Z, which is the
    /// direction the pawn travels and the direction the camera faces.
    static float RowToWorldZ(int row);

private:

    /// Appends "count" consecutive lanes of one type and advances the row.
    void AddLanes(LaneType type, int count);

    /// Builds dense, deterministic block strips around all four map edges.
    void BuildDecorations();

    std::vector<std::shared_ptr<Lane>> lanes;

    std::vector<std::shared_ptr<WorldObject>> decorations;

    /// One cube shared by every lane and every decoration, so the whole
    /// battlefield still draws from a single vertex buffer.
    std::shared_ptr<Mesh> blockMesh;

    int nextRow;

    int spawnRow;
};
