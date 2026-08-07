#pragma once

#include <array>
#include <memory>

#include <glm.hpp>

#include "Mesh.h"
#include "MeshBuilder.h"
#include "Obstacle.h"

/// Flat materials the placeholder props are painted with.
///
/// These are vertex tints rather than object colours, so one mesh can use
/// several of them and still draw in a single call: a tree is one object
/// with a brown trunk and green leaves.
namespace ObstaclePalette
{
    inline const glm::vec3 Wood = glm::vec3(0.52f, 0.36f, 0.21f);
    inline const glm::vec3 DarkWood = glm::vec3(0.36f, 0.24f, 0.14f);
    inline const glm::vec3 Leaf = glm::vec3(0.24f, 0.52f, 0.24f);
    inline const glm::vec3 DarkLeaf = glm::vec3(0.17f, 0.40f, 0.19f);
    inline const glm::vec3 Stone = glm::vec3(0.52f, 0.52f, 0.55f);
    inline const glm::vec3 DarkStone = glm::vec3(0.36f, 0.36f, 0.39f);
    inline const glm::vec3 Iron = glm::vec3(0.68f, 0.70f, 0.74f);
    inline const glm::vec3 Hide = glm::vec3(0.90f, 0.89f, 0.86f);
    inline const glm::vec3 HideDark = glm::vec3(0.24f, 0.22f, 0.22f);
    inline const glm::vec3 Mud = glm::vec3(0.30f, 0.22f, 0.12f);
    inline const glm::vec3 MudWet = glm::vec3(0.38f, 0.29f, 0.16f);
}

/// A finished prop model plus the measurements the game needs from it.
struct ObstacleModel
{
    std::shared_ptr<Mesh> mesh;

    /// Ground to the highest point of the model.
    float height = 0.0f;

    /// Footprint on the ground, used to size the shadow. Width and depth are
    /// separate because a fence is wide and shallow.
    float footprintWidth = 0.0f;

    float footprintDepth = 0.0f;

    /// Radius of a round prop, or zero for the box-shaped ones.
    ///
    /// The factory is the only place that knows how big a cannonball or a
    /// log actually is, so it reports it here rather than leaving the number
    /// to be guessed twice. MovingHazard uses it through RollingMotion now;
    /// a future circular collision bound can reuse the same value.
    float boundingRadius = 0.0f;
};

/// Builds the placeholder obstacle and hazard models.
///
/// Same rules as the chess pieces: large simple boxes, flat colours, sharp
/// edges, and nothing that does not change the silhouette. Models use a
/// consistent logical placement origin. Stationary models start at y = 0,
/// while directional assets may extend farther in the direction they face.
///
/// Airborne hazards are authored at flight height; rolling hazards touch the
/// ground. Movement and collision remain outside this visual factory.
namespace ObstacleMeshFactory
{
    //---------------------------------------------------------
    // Shared parts
    //
    // Each returns the Y it finished at, so parts chain together.
    //---------------------------------------------------------

    /// A square post. The building block of fences, palisades and legs.
    float AddPost(
        MeshBuilder& builder,
        float x,
        float z,
        float width,
        float bottomY,
        float topY);

    /// A post sharpened to a point, for palisade stakes and spikes.
    float AddSpike(
        MeshBuilder& builder,
        float x,
        float z,
        float bottomWidth,
        float topWidth,
        float bottomY,
        float topY);

    /// A horizontal shaft running along X with a stepped point on its +X
    /// end, shared by the arrow and the spear.
    void AddShaftWithTip(
        MeshBuilder& builder,
        float centerY,
        float shaftLength,
        float shaftWidth,
        const glm::vec3& shaftColor,
        const glm::vec3& tipColor);

    //---------------------------------------------------------
    // Models
    //---------------------------------------------------------

    /// To add an asset, extend its enum/count/name mapping and add one case
    /// to the matching Create overload. The mesh library, showcase loop and
    /// renderer remain type-agnostic.
    ObstacleModel Create(ObstacleType type);

    /// Fireball and Lightning intentionally return a model without a mesh;
    /// their gameplay identifiers remain available while their sprite visuals
    /// are owned by Game, outside this 3D factory.
    ObstacleModel Create(HazardType type);
}

/// Owns one model per obstacle and hazard type and hands them out.
///
/// Same contract as PieceMeshLibrary: models are built on first use and
/// shared by every prop of that type, and the library must be destroyed
/// while the GL context is still alive.
class ObstacleMeshLibrary
{
public:

    ObstacleMeshLibrary();

    const ObstacleModel& GetModel(ObstacleType type);

    const ObstacleModel& GetModel(HazardType type);

    /// Factory: a ready-to-render stationary prop.
    std::shared_ptr<StaticObstacle> CreateObstacle(ObstacleType type);

    /// Factory: a hazard visual anchor. Sprite-deferred types have a transform
    /// but no mesh or shadow, allowing movement code to remain renderer-free.
    std::shared_ptr<Hazard> CreateHazard(HazardType type);

    void Clear();

private:

    std::array<ObstacleModel, ObstacleTypeCount> obstacleModels;

    std::array<ObstacleModel, HazardTypeCount> hazardModels;
};
