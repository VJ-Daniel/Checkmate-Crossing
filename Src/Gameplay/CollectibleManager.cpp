/*
    ============================================================
    Checkmate Crossing - Collectible Manager
    Gameplay Programmer: Ayub
    Date: 2026

    See CollectibleManager.h for an overview.
    ============================================================
*/

#include "CollectibleManager.h"

CollectibleManager::CollectibleManager(PieceMeshLibrary& meshLibrary)
    : meshLibrary(meshLibrary)
{
}

void CollectibleManager::Spawn(PieceType type, const glm::vec3& groundPosition)
{
    auto piece = meshLibrary.CreatePiece(type, PieceTeam::White);
    piece->SetGroundPosition(groundPosition);

    collectibles.push_back(piece);
}

bool CollectibleManager::TryCollect(
    const glm::vec3& pawnGroundPosition,
    float pickupRadius,
    PieceType& collectedType)
{
    for (std::size_t index = 0; index < collectibles.size(); ++index)
    {
        const auto& piece = collectibles[index];

        if (!piece)
            continue;

        glm::vec3 toPawn = pawnGroundPosition - piece->GetGroundPosition();
        toPawn.y = 0.0f;

        if (glm::length(toPawn) <= pickupRadius)
        {
            collectedType = piece->GetType();

            collectibles.erase(
                collectibles.begin() + static_cast<std::ptrdiff_t>(index));

            return true;
        }
    }

    return false;
}

const std::vector<std::shared_ptr<ChessPiece>>&
    CollectibleManager::GetCollectibles() const
{
    return collectibles;
}

void CollectibleManager::Clear()
{
    collectibles.clear();
}
