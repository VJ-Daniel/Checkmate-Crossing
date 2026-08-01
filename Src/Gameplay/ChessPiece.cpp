/*
    ============================================================
    Checkmate Crossing - Chess Piece

    Which kind of piece it is and which side it belongs to. Standing on
    the ground and casting a shadow are handled by GroundEntity.

    The model itself is built by PieceMeshFactory and handed over by
    PieceMeshLibrary, so nothing about the shapes lives here.
    ============================================================
*/

#include "ChessPiece.h"

#include "GameConfig.h"
#include "PieceAnimator.h"
#include "PieceMeshFactory.h"
#include "PieceRig.h"

namespace
{
    /// An empty list to hand back for a piece with no rig, so callers can
    /// loop the result unconditionally.
    const std::vector<std::shared_ptr<SceneNode>>& NoParts()
    {
        static const std::vector<std::shared_ptr<SceneNode>> empty;

        return empty;
    }
}

PieceType PieceTypeFromIndex(int index)
{
    switch (index)
    {
    case 0:  return PieceType::Pawn;
    case 1:  return PieceType::Rook;
    case 2:  return PieceType::Knight;
    case 3:  return PieceType::Bishop;
    case 4:  return PieceType::Queen;
    case 5:  return PieceType::King;
    default: return PieceType::MountedPawn;
    }
}

const char* GetPieceTypeName(PieceType type)
{
    switch (type)
    {
    case PieceType::Pawn:   return "Pawn";
    case PieceType::Rook:   return "Rook";
    case PieceType::Knight: return "Knight";
    case PieceType::Bishop: return "Bishop";
    case PieceType::Queen:  return "Queen";
    case PieceType::King:   return "King";

    case PieceType::MountedPawn: return "MountedPawn";
    }

    return "Unknown";
}

ChessPiece::ChessPiece(
    PieceType type,
    PieceTeam team)
    : type(type),
    team(team)
{
    SetShadowScale(GameConfig::PieceShadowScale);
    SetTeam(team);
}

ChessPiece::~ChessPiece() = default;

PieceType ChessPiece::GetType() const
{
    return type;
}

void ChessPiece::AttachRig(
    std::unique_ptr<PieceRig> newRig,
    bool quadruped)
{
    rig = std::move(newRig);

    if (!rig)
    {
        animator.reset();
        return;
    }

    animator = std::make_unique<PieceAnimator>(
        quadruped ? PieceBodyPlan::Quadruped : PieceBodyPlan::Humanoid);

    // A rigged piece draws through its parts, so the baked mesh would be a
    // second copy of the same figure sitting inside the first.
    SetMesh(nullptr);

    RefreshRig();
}

bool ChessPiece::HasRig() const
{
    return rig != nullptr;
}

const std::vector<std::shared_ptr<SceneNode>>& ChessPiece::GetRigParts() const
{
    return rig ? rig->GetParts() : NoParts();
}

void ChessPiece::SetMovementState(
    bool isMoving,
    bool isGrounded,
    float newSpeedScale)
{
    moving = isMoving;
    grounded = isGrounded;
    speedScale = newSpeedScale;
}

void ChessPiece::Update(float deltaTime)
{
    if (!rig || !animator)
        return;

    animator->Update(deltaTime, moving, grounded, speedScale);

    RefreshRig();
}

void ChessPiece::SetGroundPosition(const glm::vec3& newGroundPosition)
{
    GroundEntity::SetGroundPosition(newGroundPosition);

    RefreshRig();
}

void ChessPiece::SetHeadingDegrees(float degrees)
{
    headingDegrees = degrees;

    // Written to the transform as well as the rig, so a piece turns whether
    // it draws as a hierarchy or as one baked mesh. Without this a heading
    // would silently do nothing to the pieces that have no rig, which is
    // exactly the kind of quiet inconsistency this class should not have.
    transform.SetRotation(0.0f, degrees, 0.0f);

    RefreshRig();
}

void ChessPiece::SetRigScale(float uniformScale)
{
    rigScale = uniformScale;

    RefreshRig();
}

void ChessPiece::RefreshRig()
{
    if (!rig)
        return;

    rig->SetGroundPosition(groundPosition);
    rig->SetHeadingDegrees(headingDegrees);
    rig->SetScale(rigScale);

    // Re-applied even when nothing has been animated, so a piece that is
    // simply moved still has correct world transforms before it is drawn.
    rig->ApplyPose(animator ? animator->GetPose() : PiecePose());
}

PieceTeam ChessPiece::GetTeam() const
{
    return team;
}

void ChessPiece::SetTeam(PieceTeam team)
{
    this->team = team;

    // A repainted piece carries real colours in its vertices, and the shader
    // multiplies the object colour through them. Tinting one by team would
    // wash the whole palette toward that tint and undo the paint job, so
    // those pieces are left at flat white and their materials come through
    // exactly as authored.
    //
    // A black-team version of a repainted piece is a second PieceMaterials
    // instance, not a tint - which is the reason the palette is a struct
    // rather than a set of loose constants.
    if (PieceMeshFactory::UsesDetailedMaterials(type))
    {
        SetColor(glm::vec4(1.0f));
        return;
    }

    SetColor(team == PieceTeam::White
        ? GameConfig::WhitePieceColor
        : GameConfig::BlackPieceColor);
}
