/*
    ============================================================
    Checkmate Crossing - Obstacle Mesh Factory

    Builds the placeholder props and hazards from GDD sections 2 and 4.

    Same rules as the chess pieces: large simple boxes, flat colours,
    sharp edges, and nothing that does not change the silhouette. Every
    model costs between two and six boxes and one draw call.
    ============================================================
*/

#include "ObstacleMeshFactory.h"

namespace
{
    /// Shadows sit slightly inside the footprint of a prop, which reads
    /// better than one spilling out past its edges.
    constexpr float ObstacleShadowScale = 0.95f;
}

namespace ObstacleMeshFactory
{
    float AddPost(
        MeshBuilder& builder,
        float x,
        float z,
        float width,
        float bottomY,
        float topY)
    {
        builder.AddSlabAt(x, z, width, width, bottomY, topY);

        return topY;
    }

    float AddSpike(
        MeshBuilder& builder,
        float x,
        float z,
        float bottomWidth,
        float topWidth,
        float bottomY,
        float topY)
    {
        builder.AddFrustumAt(
            x, z, bottomWidth, topWidth, bottomY, topY);

        return topY;
    }

    void AddBlob(
        MeshBuilder& builder,
        const glm::vec3& center,
        float radius)
    {
        // Three boxes crossed on the three axes. The long axis of each one
        // fills out one direction while the other two stay narrow, so the
        // corners of the overall shape are cut away and the silhouette reads
        // as round from any angle - for three boxes instead of dozens.
        const float wide = radius * 2.0f;
        const float narrow = radius * 1.42f;

        builder.AddBox(center, glm::vec3(wide, narrow, narrow));
        builder.AddBox(center, glm::vec3(narrow, wide, narrow));
        builder.AddBox(center, glm::vec3(narrow, narrow, wide));
    }

    void AddShaftWithTip(
        MeshBuilder& builder,
        float centerY,
        float shaftLength,
        float shaftWidth,
        const glm::vec3& shaftColor,
        const glm::vec3& tipColor)
    {
        builder.SetColor(shaftColor);

        builder.AddBox(
            glm::vec3(0.0f, centerY, 0.0f),
            glm::vec3(shaftLength, shaftWidth, shaftWidth));

        // The point is stepped rather than tapered. AddFrustum only narrows
        // along Y, and a projectile points along X, so two shrinking boxes
        // are both the simplest way to get a horizontal point and the more
        // voxel-looking one.
        const float shaftEnd = shaftLength * 0.5f;

        const float wideTip = shaftWidth * 2.2f;
        const float narrowTip = shaftWidth * 1.1f;

        builder.SetColor(tipColor);

        builder.AddBox(
            glm::vec3(shaftEnd + wideTip * 0.5f, centerY, 0.0f),
            glm::vec3(wideTip, wideTip, wideTip));

        builder.AddBox(
            glm::vec3(shaftEnd + wideTip + narrowTip * 0.5f, centerY, 0.0f),
            glm::vec3(narrowTip, narrowTip, narrowTip));
    }

    ObstacleModel Create(ObstacleType type)
    {
        MeshBuilder builder;

        ObstacleModel model;

        switch (type)
        {
        case ObstacleType::Tree:
        {
            // Trunk and two tapering canopy blocks. The taper is what stops
            // it reading as a lollipop on a stick.
            builder.SetColor(ObstaclePalette::DarkWood);
            AddPost(builder, 0.0f, 0.0f, 0.20f, 0.0f, 0.58f);

            builder.SetColor(ObstaclePalette::Leaf);
            builder.AddFrustum(0.85f, 0.70f, 0.52f, 1.08f);

            builder.SetColor(ObstaclePalette::DarkLeaf);
            builder.AddFrustum(0.60f, 0.34f, 1.08f, 1.55f);

            model.height = 1.55f;
            model.footprintWidth = 0.85f;
            break;
        }

        case ObstacleType::Rock:
        {
            // A wide wedge with one smaller block knocked off-centre on top.
            builder.SetColor(ObstaclePalette::Stone);
            builder.AddFrustum(0.64f, 0.46f, 0.0f, 0.32f);

            builder.SetColor(ObstaclePalette::DarkStone);
            builder.AddSlabAt(0.06f, -0.04f, 0.32f, 0.30f, 0.30f, 0.46f);

            model.height = 0.46f;
            model.footprintWidth = 0.64f;
            break;
        }

        case ObstacleType::Fence:
        {
            // Two posts, two rails. Deliberately open, so it reads as a
            // fence rather than a wall.
            builder.SetColor(ObstaclePalette::DarkWood);
            AddPost(builder, -0.42f, 0.0f, 0.11f, 0.0f, 0.72f);
            AddPost(builder, 0.42f, 0.0f, 0.11f, 0.0f, 0.72f);

            builder.SetColor(ObstaclePalette::Wood);
            builder.AddSlabAt(0.0f, 0.0f, 0.98f, 0.07f, 0.26f, 0.36f);
            builder.AddSlabAt(0.0f, 0.0f, 0.98f, 0.07f, 0.50f, 0.60f);

            model.height = 0.72f;
            model.footprintWidth = 0.98f;
            model.footprintDepth = 0.16f;
            break;
        }

        case ObstacleType::Wall:
        {
            // Solid block with an overhanging cap, which is the whole
            // difference between a wall and a plain box.
            builder.SetColor(ObstaclePalette::Stone);
            builder.AddSlabAt(0.0f, 0.0f, 0.94f, 0.28f, 0.0f, 0.64f);

            builder.SetColor(ObstaclePalette::DarkStone);
            builder.AddSlabAt(0.0f, 0.0f, 1.02f, 0.34f, 0.64f, 0.78f);

            model.height = 0.78f;
            model.footprintWidth = 1.02f;
            model.footprintDepth = 0.34f;
            break;
        }

        case ObstacleType::Bush:
        {
            builder.SetColor(ObstaclePalette::Leaf);
            builder.AddFrustum(0.66f, 0.58f, 0.0f, 0.30f);

            builder.SetColor(ObstaclePalette::DarkLeaf);
            builder.AddSlabAt(-0.04f, 0.03f, 0.44f, 0.40f, 0.28f, 0.48f);

            model.height = 0.48f;
            model.footprintWidth = 0.66f;
            break;
        }

        case ObstacleType::Spikes:
        {
            // A bed plate with four sharpened stakes, which is what the
            // frustum helper was built for.
            builder.SetColor(ObstaclePalette::DarkStone);
            builder.AddSlabAt(0.0f, 0.0f, 0.78f, 0.78f, 0.0f, 0.08f);

            builder.SetColor(ObstaclePalette::Iron);

            for (int ix = -1; ix <= 1; ix += 2)
            {
                for (int iz = -1; iz <= 1; iz += 2)
                {
                    AddSpike(
                        builder,
                        static_cast<float>(ix) * 0.19f,
                        static_cast<float>(iz) * 0.19f,
                        0.17f, 0.03f,
                        0.06f, 0.48f);
                }
            }

            model.height = 0.48f;
            model.footprintWidth = 0.78f;
            break;
        }

        case ObstacleType::Cow:
        {
            // Four legs, a body and a head. Nothing else: horns, ears and
            // markings all cost boxes without changing the silhouette.
            builder.SetColor(ObstaclePalette::HideDark);

            for (int ix = 0; ix < 2; ++ix)
            {
                for (int iz = -1; iz <= 1; iz += 2)
                {
                    AddPost(
                        builder,
                        (ix == 0) ? -0.26f : 0.22f,
                        static_cast<float>(iz) * 0.14f,
                        0.12f,
                        0.0f, 0.30f);
                }
            }

            builder.SetColor(ObstaclePalette::Hide);
            builder.AddSlabAt(-0.02f, 0.0f, 0.78f, 0.42f, 0.28f, 0.66f);
            builder.AddSlabAt(0.46f, 0.0f, 0.28f, 0.30f, 0.38f, 0.68f);

            model.height = 0.68f;
            model.footprintWidth = 0.92f;
            model.footprintDepth = 0.46f;
            break;
        }

        case ObstacleType::Palisade:
        {
            // A row of sharpened stakes lashed together. The binding rail is
            // what separates it from five unrelated spikes.
            builder.SetColor(ObstaclePalette::Wood);

            for (int stake = 0; stake < 5; ++stake)
            {
                const float x = -0.38f + 0.19f * static_cast<float>(stake);

                AddSpike(builder, x, 0.0f, 0.18f, 0.06f, 0.0f, 0.92f);
            }

            builder.SetColor(ObstaclePalette::DarkWood);
            builder.AddSlabAt(0.0f, 0.07f, 1.00f, 0.06f, 0.42f, 0.52f);

            model.height = 0.92f;
            model.footprintWidth = 1.00f;
            model.footprintDepth = 0.20f;
            break;
        }
        }

        model.mesh = builder.Build();

        return model;
    }

    ObstacleModel Create(HazardType type)
    {
        MeshBuilder builder;

        ObstacleModel model;

        switch (type)
        {
        case HazardType::Arrow:
        {
            // Hazards are authored at the height they will eventually fly
            // at, so they hover instead of resting on the ground.
            const float flightHeight = 0.40f;

            AddShaftWithTip(
                builder,
                flightHeight,
                0.62f, 0.05f,
                ObstaclePalette::Wood,
                ObstaclePalette::Iron);

            // One upright fin. Two would be hidden behind each other from
            // the gameplay camera anyway.
            builder.SetColor(ObstaclePalette::DarkWood);
            builder.AddSlabAt(
                -0.26f, 0.0f,
                0.14f, 0.03f,
                flightHeight - 0.08f, flightHeight + 0.08f);

            model.height = flightHeight + 0.08f;
            model.footprintWidth = 0.50f;
            model.footprintDepth = 0.20f;
            break;
        }

        case HazardType::Spear:
        {
            const float flightHeight = 0.44f;

            AddShaftWithTip(
                builder,
                flightHeight,
                0.86f, 0.07f,
                ObstaclePalette::Wood,
                ObstaclePalette::Iron);

            model.height = flightHeight + 0.08f;
            model.footprintWidth = 0.62f;
            model.footprintDepth = 0.20f;
            break;
        }

        case HazardType::Cannonball:
        {
            const float radius = 0.17f;
            const float flightHeight = 0.32f;

            builder.SetColor(ObstaclePalette::DarkStone);

            AddBlob(
                builder,
                glm::vec3(0.0f, flightHeight, 0.0f),
                radius);

            model.height = flightHeight + radius;
            model.footprintWidth = radius * 2.0f;
            break;
        }

        case HazardType::RollingRock:
        {
            // Larger than the airborne cannonball and touching the ground,
            // so its silhouette communicates the GDD's slower, heavier roll.
            const float radius = 0.28f;

            builder.SetColor(ObstaclePalette::Stone);

            AddBlob(
                builder,
                glm::vec3(0.0f, radius, 0.0f),
                radius);

            model.height = radius * 2.0f;
            model.footprintWidth = radius * 2.0f;
            break;
        }

        case HazardType::RollingLog:
        {
            // A long timber across X, ready to roll along a lane on Z later.
            // Oversized end caps keep it readable as a log instead of a beam.
            const float halfHeight = 0.17f;

            builder.SetColor(ObstaclePalette::Wood);
            builder.AddBox(
                glm::vec3(0.0f, halfHeight, 0.0f),
                glm::vec3(0.76f, 0.30f, 0.30f));

            builder.SetColor(ObstaclePalette::DarkWood);

            for (int end = -1; end <= 1; end += 2)
            {
                builder.AddBox(
                    glm::vec3(
                        static_cast<float>(end) * 0.40f,
                        halfHeight,
                        0.0f),
                    glm::vec3(0.08f, 0.34f, 0.34f));
            }

            model.height = halfHeight * 2.0f;
            model.footprintWidth = 0.84f;
            model.footprintDepth = 0.34f;
            break;
        }
        }

        model.mesh = builder.Build();

        return model;
    }
}

ObstacleMeshLibrary::ObstacleMeshLibrary()
{
}

const ObstacleModel& ObstacleMeshLibrary::GetModel(ObstacleType type)
{
    ObstacleModel& model = obstacleModels[static_cast<int>(type)];

    if (!model.mesh)
        model = ObstacleMeshFactory::Create(type);

    return model;
}

const ObstacleModel& ObstacleMeshLibrary::GetModel(HazardType type)
{
    ObstacleModel& model = hazardModels[static_cast<int>(type)];

    if (!model.mesh)
        model = ObstacleMeshFactory::Create(type);

    return model;
}

std::shared_ptr<StaticObstacle> ObstacleMeshLibrary::CreateObstacle(
    ObstacleType type)
{
    const ObstacleModel& model = GetModel(type);

    auto obstacle = std::make_shared<StaticObstacle>(type);

    obstacle->SetMesh(model.mesh);

    // Flat white, so the mesh's own vertex colours come through unchanged.
    obstacle->SetColor(glm::vec4(1.0f));

    obstacle->SetDimensions(
        model.height,
        model.footprintWidth,
        model.footprintDepth);

    obstacle->SetShadowScale(ObstacleShadowScale);
    obstacle->Initialize();

    return obstacle;
}

std::shared_ptr<Hazard> ObstacleMeshLibrary::CreateHazard(HazardType type)
{
    const ObstacleModel& model = GetModel(type);

    auto hazard = std::make_shared<Hazard>(type);

    hazard->SetMesh(model.mesh);
    hazard->SetColor(glm::vec4(1.0f));

    hazard->SetDimensions(
        model.height,
        model.footprintWidth,
        model.footprintDepth);

    hazard->SetShadowScale(ObstacleShadowScale);
    hazard->Initialize();

    return hazard;
}

void ObstacleMeshLibrary::Clear()
{
    for (ObstacleModel& model : obstacleModels)
        model = ObstacleModel();

    for (ObstacleModel& model : hazardModels)
        model = ObstacleModel();
}
