#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "ChessPiece.h"
#include "PieceMeshFactory.h"

/*
    ============================================================
    Checkmate Crossing - Collectible Manager
    Gameplay Programmer: Ayub
    Date: 2026

    Description:
    Owns the ally chess pieces (Bishop/Knight/Rook/Queen) waiting to be
    picked up on the field.

    Placement -- which piece goes where, on which level -- is level-design
    data; Liyuu's job, same as with HazardManager. Real pickup detection is
    Kaung's collision system, which should call Pawn::CollectPiece(type)
    directly once it exists. TryCollect() below is a placeholder standing
    in for that (see the note on it and in Game::Update).
    ============================================================
*/
class CollectibleManager
{
public:

    explicit CollectibleManager(PieceMeshLibrary& meshLibrary);

    /// Places one collectible ally on the field.
    void Spawn(PieceType type, const glm::vec3& groundPosition);

    /// TEMPORARY stand-in for Kaung's collision detection: if any
    /// collectible is within pickupRadius of pawnGroundPosition, removes
    /// it, writes its type to collectedType, and returns true. Delete
    /// this once real collision detection calls Pawn::CollectPiece
    /// directly and removes the piece itself.
    bool TryCollect(
        const glm::vec3& pawnGroundPosition,
        float pickupRadius,
        PieceType& collectedType);

    const std::vector<std::shared_ptr<ChessPiece>>& GetCollectibles() const;

    void Clear();

private:

    PieceMeshLibrary& meshLibrary;

    std::vector<std::shared_ptr<ChessPiece>> collectibles;
};
