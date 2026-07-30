/*
    ============================================================
    Checkmate Crossing - Ground Shadow

    The fake shadow shared by every character that stands on the
    battlefield: a flat translucent quad kept underneath its owner.
    ============================================================
*/

#include "GroundShadow.h"

#include "GameConfig.h"

GroundShadow::GroundShadow()
    : footprintWidth(1.0f),
    footprintDepth(1.0f)
{
    color = GameConfig::ShadowColor;
}

void GroundShadow::Initialize()
{
    // A flat quad rather than a cube: it lies on the ground and only ever
    // needs its top face.
    SetMesh(Mesh::CreateGroundPlane());

    SetFootprint(footprintWidth, footprintDepth);
}

void GroundShadow::SetFootprint(float width, float depth)
{
    footprintWidth = width;
    footprintDepth = (depth > 0.0f) ? depth : width;

    transform.SetScale(footprintWidth, 1.0f, footprintDepth);
}

void GroundShadow::PlaceOn(const glm::vec3& groundPosition)
{
    transform.SetPosition(
        groundPosition.x,
        groundPosition.y + GameConfig::ShadowGroundOffset,
        groundPosition.z);
}
