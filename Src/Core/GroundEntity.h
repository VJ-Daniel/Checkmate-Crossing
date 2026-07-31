#pragma once

#include <glm.hpp>

#include "GroundShadow.h"
#include "WorldObject.h"

/// Base for anything that stands on the battlefield.
///
/// It knows the point of ground it occupies, how big its model is, and it
/// carries the fake shadow that keeps it anchored there. Models handled this
/// way are authored with y = 0 at their base, so placing one is just naming
/// the patch of ground it stands on - no half-height correction anywhere.
///
/// Chess pieces and obstacles both work exactly this way, which is why it
/// lives here rather than being written out twice.
class GroundEntity : public WorldObject
{
public:

    GroundEntity();

    /// Builds the shadow. Call after the dimensions are known, and only
    /// once a GL context exists.
    void Initialize() override;

    /// Stands the entity on a point of ground.
    ///
    /// Virtual because an entity made of several meshes - the checkpoint
    /// gate, and the king's cage after it - has to take its parts with it,
    /// and every caller places one the same way regardless.
    virtual void SetGroundPosition(const glm::vec3& groundPosition);

    const glm::vec3& GetGroundPosition() const;

    /// Records the size of the model, so the shadow can match it. Depth
    /// defaults to width, which is right for anything roughly square.
    void SetDimensions(
        float height,
        float footprintWidth,
        float footprintDepth = 0.0f);

    float GetHeight() const;

    float GetFootprintWidth() const;

    float GetFootprintDepth() const;

    /// How much wider than the model its shadow is drawn.
    void SetShadowScale(float scale);

    /// Turns the shadow off for visuals that should not cast one.
    void SetShadowVisible(bool visible);

    const WorldObject& GetShadow() const;

protected:

    /// Puts the shadow back under the entity's current ground position.
    void UpdateShadow();

    glm::vec3 groundPosition;

    float height;

    float footprintWidth;

    float footprintDepth;

    float shadowScale;

    GroundShadow shadow;
};
