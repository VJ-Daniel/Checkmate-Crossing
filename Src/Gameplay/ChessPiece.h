#pragma once

#include <glm.hpp>

#include "GroundEntity.h"

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

    PieceType GetType() const;

    PieceTeam GetTeam() const;

    /// Re-applies the team colour. Meshes are shared between teams, so
    /// switching sides costs nothing.
    void SetTeam(PieceTeam team);

private:

    PieceType type;

    PieceTeam team;
};
