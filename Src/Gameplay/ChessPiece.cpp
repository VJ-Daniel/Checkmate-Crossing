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

PieceType ChessPiece::GetType() const
{
    return type;
}

PieceTeam ChessPiece::GetTeam() const
{
    return team;
}

void ChessPiece::SetTeam(PieceTeam team)
{
    this->team = team;

    SetColor(team == PieceTeam::White
        ? GameConfig::WhitePieceColor
        : GameConfig::BlackPieceColor);
}
