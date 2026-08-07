#include "Collision.h"
#include "GroundEntity.h"
#include <cmath>
#include <algorithm>


// ---- CollisionBox ----

bool CollisionBox::Overlaps(const CollisionBox& a, const CollisionBox& b)
{
    // FIX: Standard AABB check using absolute distances and HALF extents.
    // If distance on any axis is greater than the sum of half extents, they do NOT overlap.
    return (std::abs(a.center.x - b.center.x) < (a.halfExtents.x + b.halfExtents.x)) &&
        (std::abs(a.center.y - b.center.y) < (a.halfExtents.y + b.halfExtents.y)) &&
        (std::abs(a.center.z - b.center.z) < (a.halfExtents.z + b.halfExtents.z));
}

bool CollisionBox::ContainsPoint(const glm::vec3& point) const
{
    return (std::abs(point.x - center.x) <= halfExtents.x) &&
        (std::abs(point.y - center.y) <= halfExtents.y) &&
        (std::abs(point.z - center.z) <= halfExtents.z);
}

float CollisionBox::DistanceToPoint(const glm::vec3& point) const
{
    glm::vec3 delta = point - center;
    float distX = std::max(0.0f, std::abs(delta.x) - halfExtents.x);
    float distY = std::max(0.0f, std::abs(delta.y) - halfExtents.y);
    float distZ = std::max(0.0f, std::abs(delta.z) - halfExtents.z);
    return std::sqrt(distX * distX + distY * distY + distZ * distZ);
}

// ---- CollisionCircle ----

bool CollisionCircle::Overlaps(const CollisionCircle& a, const CollisionCircle& b)
{
    float dist = glm::length(a.center - b.center);
    return dist < (a.radius + b.radius);
}

bool CollisionCircle::ContainsPoint(const glm::vec3& point) const
{
    return glm::length(point - center) <= radius;
}

float CollisionCircle::DistanceToPoint(const glm::vec3& point) const
{
    return std::max(0.0f, glm::length(point - center) - radius);
}

// ---- CollisionComponent ----

CollisionComponent::CollisionComponent()
{
    box.center = glm::vec3(0.0f);
    box.halfExtents = glm::vec3(0.5f);
    circle.center = glm::vec3(0.0f);
    circle.radius = 0.5f;
}

void CollisionComponent::SetBox(const glm::vec3& center, const glm::vec3& halfExtents)
{
    useBox = true;
    useCircle = false;
    box.center = center;
    box.halfExtents = halfExtents;
}

void CollisionComponent::SetCircle(const glm::vec3& center, float radius)
{
    useBox = false;
    useCircle = true;
    circle.center = center;
    circle.radius = radius;
}

bool CollisionComponent::Intersects(const CollisionComponent& other) const
{
    if (useBox && other.useBox)
        return CollisionBox::Overlaps(box, other.box);
    if (useCircle && other.useCircle)
        return CollisionCircle::Overlaps(circle, other.circle);
    if (useBox && other.useCircle)
        return other.circle.ContainsPoint(box.center) ||
        box.DistanceToPoint(other.circle.center) <= 0.0f;
    if (useCircle && other.useBox)
        return circle.ContainsPoint(other.box.center) ||
        other.box.DistanceToPoint(circle.center) <= 0.0f;
    return false;
}

bool CollisionComponent::Intersects(const CollisionBox& otherBox) const
{
    if (useBox)
        return CollisionBox::Overlaps(box, otherBox);
    if (useCircle)
        return otherBox.DistanceToPoint(circle.center) <= 0.0f;
    return false;
}

bool CollisionComponent::Intersects(const CollisionCircle& otherCircle) const
{
    if (useCircle)
        return CollisionCircle::Overlaps(circle, otherCircle);
    if (useBox)
        return otherCircle.ContainsPoint(box.center) ||
        box.DistanceToPoint(otherCircle.center) <= 0.0f;
    return false;
}

bool CollisionComponent::ContainsPoint(const glm::vec3& point) const
{
    if (useBox)
        return box.ContainsPoint(point);
    if (useCircle)
        return circle.ContainsPoint(point);
    return false;
}

glm::vec3 CollisionComponent::GetNearestPoint(const glm::vec3& point) const
{
    if (useBox)
    {
        glm::vec3 clamped = glm::clamp(point, box.center - box.halfExtents, box.center + box.halfExtents);
        return clamped;
    }
    if (useCircle)
    {
        glm::vec3 dir = point - circle.center;
        float length = glm::length(dir);
        if (length < 0.0001f)
            return circle.center + glm::vec3(circle.radius, 0.0f, 0.0f);
        return circle.center + (dir / length) * circle.radius;
    }
    return point;
}

void CollisionComponent::SetPosition(const glm::vec3& position)
{
    if (useBox)
    {
        // FIX: Position is absolute. DO NOT add any offsets!
        // The box center is exactly where the entity's GroundPosition is.
        box.center = position;
    }
    if (useCircle)
    {
        // FIX: Position is absolute.
        circle.center = position;
    }
}

void CollisionComponent::UpdateFromEntity(const GroundEntity& entity, float scale)
{
    // Use the existing GroundEntity methods
    glm::vec3 position = entity.GetGroundPosition();
    float height = entity.GetHeight();
    float width = entity.GetFootprintWidth() * scale;
    float depth = entity.GetFootprintDepth() * scale;

    // If depth wasn't set, use width
    if (depth <= 0.0f)
        depth = width;

    // FIX: REMOVED the + (height * 0.5f) from the center.
    // The collision box sits exactly at the base (GroundPosition).
    glm::vec3 center = position;

    // For round-ish entities, use circle
    bool isRound = (std::abs(width - depth) < 0.001f && width > 0.1f);

    if (isRound)
    {
        float radius = width * 0.5f;
        SetCircle(center, radius);
    }
    else
    {
        SetBox(center, glm::vec3(width * 0.5f, height * 0.5f, depth * 0.5f));
    }
}

glm::vec3 CollisionComponent::ResolveHorizontalOverlap(
    const CollisionComponent& other) const
{
    // Both volumes are treated as their bounding boxes. A circle's box is a
    // fair stand-in at this scale and keeps one code path for the resolve.
    const glm::vec3 aCenter = useBox ? box.center : circle.center;
    const glm::vec3 aHalf = useBox
        ? box.halfExtents
        : glm::vec3(circle.radius, circle.radius, circle.radius);

    const glm::vec3 bCenter = other.useBox
        ? other.box.center
        : other.circle.center;
    const glm::vec3 bHalf = other.useBox
        ? other.box.halfExtents
        : glm::vec3(other.circle.radius, other.circle.radius,
            other.circle.radius);

    const float deltaX = aCenter.x - bCenter.x;
    const float deltaZ = aCenter.z - bCenter.z;

    const float overlapX = (aHalf.x + bHalf.x) - std::abs(deltaX);
    const float overlapZ = (aHalf.z + bHalf.z) - std::abs(deltaZ);

    // Not touching on one axis means not touching at all.
    if (overlapX <= 0.0f || overlapZ <= 0.0f)
        return glm::vec3(0.0f);

    // Leave along whichever axis is least buried. That is what makes walking
    // into a wall face push straight back out rather than sliding round it.
    if (overlapX < overlapZ)
    {
        const float sign = (deltaX < 0.0f) ? -1.0f : 1.0f;
        return glm::vec3(overlapX * sign, 0.0f, 0.0f);
    }

    const float sign = (deltaZ < 0.0f) ? -1.0f : 1.0f;
    return glm::vec3(0.0f, 0.0f, overlapZ * sign);
}
