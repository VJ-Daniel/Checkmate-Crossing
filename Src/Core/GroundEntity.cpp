/*
    ============================================================
    Checkmate Crossing - Ground Entity

    Shared behaviour for everything that stands on the battlefield:
    where it stands, how big it is, and the shadow underneath it.
    ============================================================
*/

#include "GroundEntity.h"

#include "GameConfig.h"

GroundEntity::GroundEntity()
    : groundPosition(0.0f),
    height(1.0f),
    footprintWidth(0.5f),
    footprintDepth(0.5f),
    shadowScale(GameConfig::PieceShadowScale)
{
}

void GroundEntity::Initialize()
{
    shadow.SetFootprint(
        footprintWidth * shadowScale,
        footprintDepth * shadowScale);

    shadow.Initialize();

    UpdateShadow();
}

void GroundEntity::SetGroundPosition(const glm::vec3& groundPosition)
{
    this->groundPosition = groundPosition;

    // The model's pivot is already at its base, so the ground point is the
    // position with no correction.
    transform.SetPosition(groundPosition);

    UpdateShadow();
}

const glm::vec3& GroundEntity::GetGroundPosition() const
{
    return groundPosition;
}

void GroundEntity::SetDimensions(
    float height,
    float footprintWidth,
    float footprintDepth)
{
    this->height = height;
    this->footprintWidth = footprintWidth;
    this->footprintDepth =
        (footprintDepth > 0.0f) ? footprintDepth : footprintWidth;

    shadow.SetFootprint(
        this->footprintWidth * shadowScale,
        this->footprintDepth * shadowScale);
}

float GroundEntity::GetHeight() const
{
    return height;
}

float GroundEntity::GetFootprintWidth() const
{
    return footprintWidth;
}

float GroundEntity::GetFootprintDepth() const
{
    return footprintDepth;
}

void GroundEntity::SetShadowScale(float scale)
{
    shadowScale = scale;

    shadow.SetFootprint(
        footprintWidth * shadowScale,
        footprintDepth * shadowScale);
}

void GroundEntity::SetShadowVisible(bool visible)
{
    shadow.SetVisible(visible);
}

const WorldObject& GroundEntity::GetShadow() const
{
    return shadow;
}

void GroundEntity::UpdateShadow()
{
    shadow.PlaceOn(groundPosition);
}
