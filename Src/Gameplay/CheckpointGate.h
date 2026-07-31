#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "GateMeshFactory.h"
#include "GroundEntity.h"
#include "WorldObject.h"

/// One rigid piece of a gate: a shared mesh, its own transform, and the
/// offset it keeps from the gate it belongs to.
///
/// This is the smallest thing that will do the job of a parent/child
/// transform, and it exists for exactly one reason. Voxel models in this
/// project are baked into a single mesh precisely so no scene graph is
/// needed - but a gate whose two leaves have to turn on their own hinges
/// needs at least one level of one. Each piece remembers where it belongs,
/// so moving the whole gate is one pass over the pieces and turning a leaf
/// touches nothing but that leaf.
class GatePiece : public WorldObject
{
public:

    GatePiece();

    /// Where this piece sits relative to the gate's own ground position.
    void SetLocalOffset(const glm::vec3& offset);

    const glm::vec3& GetLocalOffset() const;

    /// Puts the piece back where it belongs, leaving whatever rotation it is
    /// already holding alone - which is what lets a door keep its angle
    /// while the gate around it is moved.
    void PlaceRelativeTo(const glm::vec3& gateGroundPosition);

private:

    glm::vec3 localOffset;
};

/// Which of the optional dressing a gate is built with.
///
/// The gate is a kit, so the pieces that are not structural are switchable:
/// a plain gateway with no wall run is what a doorway in a courtyard wants,
/// and the full run is what a checkpoint across the battlefield wants.
struct CheckpointGateLayout
{
    /// Wall segments running out from each pillar. Four carries the run
    /// clear of the walkable board and on into the scenery border, so the
    /// wall disappears behind the trees rather than visibly stopping.
    int wallSegmentsPerSide = 4;

    /// The stepped capstone on each pillar.
    bool topCaps = true;

    /// The banner hanging on each pillar's front face.
    bool banners = true;
};

/// A medieval checkpoint entrance: two pillars, two hinged leaves, and a
/// stone wall running out from either side.
///
/// The gate stands on the ground like any other prop, but unlike one it is
/// several meshes rather than one, because its leaves have to move
/// independently. It owns those pieces and places them; the renderer draws
/// them the same way it draws everything else.
///
/// Nothing here animates. The leaves are posed, not driven: SetDoorAngle
/// puts them at an angle and leaves them there. What this class guarantees
/// is that the angle is the only thing an animation will ever have to write
/// - the pivots, the mirroring and the limits are already correct.
class CheckpointGate : public GroundEntity
{
public:

    CheckpointGate();

    /// Assembles the gate from the library's shared part meshes.
    ///
    /// Needs a live GL context, the same as every other model. Safe to call
    /// again to rebuild the gate with a different layout.
    void Build(
        GateMeshLibrary& meshes,
        const CheckpointGateLayout& layout = CheckpointGateLayout());

    /// Stands the whole structure on a point of ground, taking every piece
    /// with it and leaving the leaves at the angle they were posed at.
    void SetGroundPosition(const glm::vec3& groundPosition) override;

    //---------------------------------------------------------
    // Doors
    //---------------------------------------------------------

    /// Swings both leaves open by the given angle, in degrees.
    ///
    /// Zero is shut, and the leaves meet exactly on the centre line of the
    /// gateway. The maximum lays each leaf flat along its own pillar. The
    /// two take opposite signs, so they open away from one another like a
    /// pair of castle gates rather than sweeping the same way.
    void SetDoorAngle(float degrees);

    float GetDoorAngle() const;

    /// Widest the leaves may swing. Anything past this stops behaving like
    /// a door, so SetDoorAngle clamps to it.
    static float GetMaxDoorAngle();

    /// Clear span of the gateway, with both leaves out of the way.
    static float GetOpeningWidth();

    /// The leaves themselves, for anything that needs to drive them
    /// directly. Null until Build has run.
    GatePiece* GetLeftDoor();

    GatePiece* GetRightDoor();

    //---------------------------------------------------------
    // Rendering
    //---------------------------------------------------------

    /// Every piece of the gate, in no particular order: the depth buffer
    /// sorts them. The gate's own shadow comes from GroundEntity and is
    /// drawn with the other shadows, before any of these.
    const std::vector<std::shared_ptr<GatePiece>>& GetParts() const;

    const CheckpointGateLayout& GetLayout() const;

private:

    /// Creates one piece from a part model and files it under the gate.
    ///
    /// shade tints the whole piece. It is left at one for everything
    /// structural and only used to stop a long wall run looking like the
    /// same block stamped out nine times.
    std::shared_ptr<GatePiece> AddPart(
        const GatePartModel& model,
        const glm::vec3& offset,
        float shade = 1.0f);

    CheckpointGateLayout layout;

    std::vector<std::shared_ptr<GatePiece>> parts;

    /// Also held in parts. Kept separately because they are the only two
    /// pieces anything outside this class has a reason to touch.
    std::shared_ptr<GatePiece> leftDoor;

    std::shared_ptr<GatePiece> rightDoor;

    float doorAngle;
};
