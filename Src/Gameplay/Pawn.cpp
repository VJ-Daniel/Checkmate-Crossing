/*
    ============================================================
    Checkmate Crossing - Pawn

    The player's piece as a placeholder cube, matching prototype step 2
    of GDD section 5. No gameplay behaviour is implemented yet.
    ============================================================
*/

#include "Pawn.h"

#include "GameConfig.h"

Pawn::Pawn()
    : spawnPosition(0.0f),
    groundHeight(0.0f)
{
}

void Pawn::Initialize()
{
    SetMesh(Mesh::CreateCube());

    SetColor(GameConfig::PawnColor);

    shadow.SetFootprint(GameConfig::PawnWidth * GameConfig::ShadowScale);
    shadow.Initialize();

    // Slightly taller than it is wide, so the piece reads as standing up
    // rather than as a floor tile.
    transform.SetScale(
        GameConfig::PawnWidth,
        GameConfig::PawnHeight,
        GameConfig::PawnWidth);

    transform.SetRotation(0.0f, 0.0f, 0.0f);

    Respawn();
}

void Pawn::SetSpawnPosition(const glm::vec3& groundPosition)
{
    spawnPosition = groundPosition;
}

const glm::vec3& Pawn::GetSpawnPosition() const
{
    return spawnPosition;
}

void Pawn::Respawn()
{
    // The spawn point is a point on the ground, and the cube mesh is centred
    // on its own origin, so lifting it by half its height rests its base
    // exactly on that surface.
    groundHeight = spawnPosition.y;

    transform.SetPosition(
        spawnPosition.x,
        spawnPosition.y + GameConfig::PawnHeight * 0.5f,
        spawnPosition.z);

    UpdateShadow();
}

const WorldObject& Pawn::GetShadow() const
{
    return shadow;
}

void Pawn::UpdateShadow()
{
    const glm::vec3& position = transform.GetPosition();

    shadow.PlaceOn(
        glm::vec3(position.x, groundHeight, position.z));
}
