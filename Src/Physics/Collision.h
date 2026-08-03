#pragma once

#include <glm.hpp>
#include <vector>
#include <memory>

class GroundEntity;
class Pawn;

/// AABB collision volume with world-space extents
struct CollisionBox
{
    glm::vec3 center;
    glm::vec3 halfExtents; // width/2, height/2, depth/2

    /// Checks overlap between two AABBs
    static bool Overlaps(const CollisionBox& a, const CollisionBox& b);

    /// Checks if a point is inside the box
    bool ContainsPoint(const glm::vec3& point) const;
    /// Returns the minimum distance from a point to the box surface
    float DistanceToPoint(const glm::vec3& point) const;
};

/// Circular collision volume (for area effects)
struct CollisionCircle
{
    glm::vec3 center;
    float radius;

    /// Checks overlap between two circles
    static bool Overlaps(const CollisionCircle& a, const CollisionCircle& b);

    /// Checks if a point is inside the circle
    bool ContainsPoint(const glm::vec3& point) const;

    /// Returns distance from center to point
    float DistanceToPoint(const glm::vec3& point) const;
};

/// Manages collision detection for a single entity
class CollisionComponent
{
public:
    CollisionComponent();

    /// Set the collision shape (AABB)
    void SetBox(const glm::vec3& center, const glm::vec3& halfExtents);

    /// Set the collision shape (circle)
    void SetCircle(const glm::vec3& center, float radius);

    /// Get the current collision shape
    const CollisionBox& GetBox() const { return box; }
    const CollisionCircle& GetCircle() const { return circle; }

    /// Check if this component collides with another
    bool Intersects(const CollisionComponent& other) const;
    bool Intersects(const CollisionBox& otherBox) const;
    bool Intersects(const CollisionCircle& otherCircle) const;

    /// Check if a point is inside this collision volume
    bool ContainsPoint(const glm::vec3& point) const;

    /// Get the nearest point on the collision volume surface
    glm::vec3 GetNearestPoint(const glm::vec3& point) const;

    /// The shortest horizontal push that separates this volume from another.
    ///
    /// Zero when they are not actually overlapping, so a caller can use it as
    /// its own "am I blocked" test.
    ///
    /// This is the fix for the wall jitter. The previous resolution moved the
    /// pawn to nearest + normalize(pos - nearest) * (|pos - nearest| + skin),
    /// which is the pawn's own position plus that whole distance again - an
    /// ejection roughly twice as far as the overlap, not a resolution. Held
    /// against a wall the pawn was thrown clear, walked back in next frame,
    /// and was thrown clear again: the oscillation that reads as shaking.
    ///
    /// Pushing out by exactly the penetration depth leaves the pawn resting
    /// flush on the surface, so the following frame has nothing left to
    /// resolve and the position is stable.
    ///
    /// Only X and Z are considered. The pawn's height belongs to the terrain
    /// snap and the jump arc, and pushing it up out of a wall would let the
    /// player climb one.
    glm::vec3 ResolveHorizontalOverlap(const CollisionComponent& other) const;

    /// Update position of the collision volume
    void SetPosition(const glm::vec3& position);

    /// Update from a GroundEntity's transform
    void UpdateFromEntity(const GroundEntity& entity, float scale = 1.0f);

    bool IsBox() const { return useBox; }
    bool IsCircle() const { return useCircle; }

private:
    bool useBox = true;
    bool useCircle = false;
    CollisionBox box;
    CollisionCircle circle;
};