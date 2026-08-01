#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "GroundEntity.h"

class PieceRig;
class PieceAnimator;
class SceneNode;

/// Forward-declared rather than included: PieceRig reaches PieceMeshFactory,
/// which needs PieceType from this header. Holding the two behind pointers
/// breaks that cycle without moving the enum into a header of its own.

/// The six kinds of chess piece in the game.
///
/// The player is a pawn, allies are collected as bishops, knights, rooks and
/// queens, and the king is the rescue target (GDD section 3).
enum class PieceType
{
    Pawn,
    Rook,
    Knight,
    Bishop,
    Queen,
    King,

    /// A pawn riding the knight's horse. Appended last so the indices of the
    /// six standard pieces are unchanged.
    MountedPawn
};

/// Which side a piece belongs to. Only decides its colour for now.
enum class PieceTeam
{
    White,
    Black
};

/// Number of entries in PieceType, for iterating over every kind.
constexpr int PieceTypeCount = 7;

/// Converts an index in [0, PieceTypeCount) to a PieceType.
PieceType PieceTypeFromIndex(int index);

/// Human-readable name, useful for logging and later for UI labels.
const char* GetPieceTypeName(PieceType type);

/// A chess piece standing on the battlefield.
///
/// The model is a single baked mesh built from cubes, shared by every piece
/// of the same type, so a piece costs exactly one draw call. Standing on the
/// ground and casting a shadow comes from GroundEntity.
///
/// This is deliberately model-and-placement only. Movement, collision and
/// the chess abilities from the GDD all belong in subclasses or components
/// added later; nothing here needs to change to make room for them.
class ChessPiece : public GroundEntity
{
public:

    ChessPiece(
        PieceType type,
        PieceTeam team = PieceTeam::White);

    /// Out of line, so the forward-declared members above can be destroyed.
    ~ChessPiece() override;

    PieceType GetType() const;

    PieceTeam GetTeam() const;

    /// Re-applies the team colour. Meshes are shared between teams, so
    /// switching sides costs nothing.
    void SetTeam(PieceTeam team);

    //-----------------------------------------------------------
    // Animation
    //
    // Optional. A piece with a rig draws as a hierarchy of parts and can be
    // animated; one without still draws as a single baked mesh, which is all
    // a piece that never moves needs. Both go through the same renderer.
    //-----------------------------------------------------------

    /// Gives this piece an animated rig. Needs a live GL context.
    void AttachRig(
        std::unique_ptr<PieceRig> rig,
        bool quadruped);

    bool HasRig() const;

    /// The parts to draw, or an empty list when this piece has no rig and
    /// should be drawn as an ordinary WorldObject instead.
    const std::vector<std::shared_ptr<SceneNode>>& GetRigParts() const;

    /// Tells the animation what the piece is doing.
    ///
    /// Movement state is pushed in rather than read out of anything, which
    /// is what keeps this class free of any opinion about how a piece moves.
    /// A piece nobody calls this on simply stands still.
    void SetMovementState(
        bool isMoving,
        bool isGrounded,
        float speedScale = 1.0f);

    /// Advances the animation. Does nothing without a rig.
    void Update(float deltaTime) override;

    /// Keeps the rig standing where the piece does.
    void SetGroundPosition(const glm::vec3& groundPosition) override;

    /// Faces the piece along a heading in degrees, for anything that walks.
    void SetHeadingDegrees(float degrees);

    /// Uniform scale applied to the rig, matching the transform scale a
    /// baked piece is given.
    void SetRigScale(float uniformScale);

private:

    void RefreshRig();

    PieceType type;

    PieceTeam team;

    std::unique_ptr<PieceRig> rig;

    std::unique_ptr<PieceAnimator> animator;

    bool moving = false;

    bool grounded = true;

    float speedScale = 1.0f;

    float headingDegrees = 0.0f;

    float rigScale = 1.0f;
};
