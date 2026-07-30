#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "Lane.h"
#include "Mesh.h"

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

    /// Scenery blocks standing outside the walkable board.
    ///
    /// Purely visual: they give the orthographic view something with real
    /// height to shade, which is what stops an empty map from looking like
    /// a flat striped board. Nothing here is an obstacle.
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

    /// How far from the centre the pawn may walk, for later clamping.
    static float GetPlayableHalfWidth();

    /// Converts a row index into its world-space Z.
    /// Row 0 sits at the origin and the level runs toward -Z, which is the
    /// direction the pawn travels and the direction the camera faces.
    static float RowToWorldZ(int row);

private:

    /// Appends "count" consecutive lanes of one type and advances the row.
    void AddLanes(LaneType type, int count);

    /// Scatters scenery blocks down both sides of the finished level.
    void BuildDecorations();

    std::vector<std::shared_ptr<Lane>> lanes;

    std::vector<std::shared_ptr<WorldObject>> decorations;

    /// One cube shared by every lane and every decoration, so the whole
    /// battlefield still draws from a single vertex buffer.
    std::shared_ptr<Mesh> blockMesh;

    int nextRow;

    int spawnRow;
};
