// GroundEntity.cpp
#include "GroundEntity.h"

GroundEntity::GroundEntity()
    : groundPosition(0.0f)
    , height(0.0f)
    , footprintWidth(0.0f)
    , footprintDepth(0.0f)
    , shadowScale(1.0f)
{
}

void GroundEntity::Initialize()
{
    // Initialize the shadow
    shadow.Initialize();
}

void GroundEntity::SetGroundPosition(const glm::vec3& groundPosition)
{
    this->groundPosition = groundPosition;

    // KEEP THIS LINE: This makes the visual model appear!
    transform.SetPosition(groundPosition);

    UpdateShadow();
}

const glm::vec3& GroundEntity::GetGroundPosition() const
{
    return groundPosition;
}

void GroundEntity::SetDimensions(float height, float footprintWidth, float footprintDepth)
{
    this->height = height;
    this->footprintWidth = footprintWidth;
    this->footprintDepth = (footprintDepth > 0.0f) ? footprintDepth : footprintWidth;
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