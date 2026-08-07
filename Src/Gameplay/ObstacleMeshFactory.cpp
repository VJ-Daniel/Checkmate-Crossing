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
            //
            // Tall enough to read as a barrier rather than a kerb: the pawn
            // stands 0.9, so the old 0.78 top sat below its shoulder and the
            // wall looked like something to step over. Thickness, width and
            // the cap's overhang are unchanged - only the body grew.
            builder.SetColor(ObstaclePalette::Stone);
            builder.AddSlabAt(0.0f, 0.0f, 0.94f, 0.28f, 0.0f, 0.56f);

            // Second course, a shade darker. Without it the taller body is
            // one flat slab; with it the wall keeps the same block scale it
            // had before and simply stacks another row.
            builder.SetColor(ObstaclePalette::Stone * 0.92f);
            builder.AddSlabAt(0.0f, 0.0f, 0.90f, 0.28f, 0.56f, 1.12f);

            builder.SetColor(ObstaclePalette::DarkStone);
            builder.AddSlabAt(0.0f, 0.0f, 1.02f, 0.34f, 1.12f, 1.28f);

            // Reported from the geometry, so the collision box Kaung's
            // system builds from GetHeight() grows with the wall.
            model.height = 1.28f;
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

            model.height = 0.75f;
            model.footprintWidth = 1.00f;
            model.footprintDepth = 0.20f;
            break;
        }

        case ObstacleType::Mud:
        {
            // A flat, low puddle patch. Deliberately far shorter than every
            // other stationary prop -- the GDD has mud slow the player, not
            // block them, so nothing about its silhouette should read as an
            // obstacle to jump.
            builder.SetColor(ObstaclePalette::Mud);
            builder.AddSlabAt(0.0f, 0.0f, 0.95f, 0.85f, 0.0f, 0.05f);

            builder.SetColor(ObstaclePalette::MudWet);
            builder.AddSlabAt(0.18f, -0.08f, 0.42f, 0.38f, 0.03f, 0.07f);
            builder.AddSlabAt(-0.22f, 0.14f, 0.30f, 0.28f, 0.02f, 0.06f);

            model.height = 0.07f;
            model.footprintWidth = 0.95f;
            model.footprintDepth = 0.85f;
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
            // A compact faceted sphere. Eight segments give an octagonal
            // outline, which is round enough to read as a ball while every
            // facet stays visible and blocky.
            const float radius = 0.17f;
            const float flightHeight = 0.32f;

            builder.SetColor(ObstaclePalette::DarkStone);

            builder.AddFacetedSphere(
                glm::vec3(0.0f, flightHeight, 0.0f),
                radius,
                8,
                6);

            model.height = flightHeight + radius;
            model.footprintWidth = radius * 2.0f;
            model.boundingRadius = radius;
            break;
        }

        case HazardType::RollingRock:
        {
            // Larger than the airborne cannonball and touching the ground,
            // so its silhouette communicates the GDD's slower, heavier roll.
            //
            // Nine segments rather than eight, plus a little corner
            // displacement, so it is visibly round but never symmetrical -
            // an even count would line the facets up front to back and make
            // it look machined.
            const float radius = 0.28f;
            const float irregularity = 0.14f;

            builder.SetColor(ObstaclePalette::Stone);

            builder.AddFacetedSphere(
                glm::vec3(0.0f, radius, 0.0f),
                radius,
                9,
                6,
                irregularity,
                1337u);

            // The displacement fades out at the poles, so the rock still
            // touches the ground at exactly y = 0 and stands 2r tall.
            model.height = radius * 2.0f;
            model.footprintWidth = radius * 2.0f * (1.0f + irregularity);
            model.boundingRadius = radius;
            break;
        }

        case HazardType::RollingLog:
        {
            // A ten-sided timber whose length runs along X, across its
            // depth-wise path, so it rolls about its own long axis.
            //
            // RollingLog examples move along Z. Keeping the cylinder
            // perpendicular to travel lets RollingMotion rotate it about X
            // instead of sliding it along its length.
            //
            // The orientation pays off twice: it also turns one flat circular
            // end towards the camera, which is the only view in which a
            // cylinder reads as a cylinder rather than a plank.
            const float radius = 0.18f;
            const float length = 0.88f;

            builder.SetColor(ObstaclePalette::Wood);

            builder.AddFacetedCylinder(
                glm::vec3(0.0f, radius, 0.0f),
                radius,
                length,
                10,
                MeshBuilder::CylinderAxis::X);

            // A slightly proud darker ring at each end, so the cut faces read
            // as timber rather than as the ends of a dowel.
            builder.SetColor(ObstaclePalette::DarkWood);

            for (int end = -1; end <= 1; end += 2)
            {
                builder.AddFacetedCylinder(
                    glm::vec3(
                        static_cast<float>(end) *
                            (length * 0.5f - 0.035f),
                        radius,
                        0.0f),
                    radius * 1.05f,
                    0.07f,
                    10,
                    MeshBuilder::CylinderAxis::X);
            }

            model.height = radius * 2.0f;
            model.footprintWidth = length;
            model.footprintDepth = radius * 2.0f;
            model.boundingRadius = radius;
            break;
        }

        case HazardType::Fireball:
        {
            // Authored from y = 0 up, because one mesh serves two jobs: the
            // fireball sweeping across the lane, and the burn patch it
            // leaves behind (HazardManager spawns those as a temporary zone
            // of the same type). A ball authored at flight height would
            // hover above the ground as a patch.
            const float radius = 0.20f;
            const float centerY = radius;

            // Scorched core, barely visible in flight but the whole of the
            // patch once the flames have settled onto the ground.
            builder.SetColor(ObstaclePalette::Ember);
            builder.AddFacetedSphere(
                glm::vec3(0.0f, centerY, 0.0f), radius, 8, 6, 0.18f, 4242u);

            builder.SetColor(ObstaclePalette::Flame);
            builder.AddFacetedSphere(
                glm::vec3(0.0f, centerY + 0.045f, 0.0f),
                radius * 0.78f, 8, 6, 0.22f, 99u);

            builder.SetColor(ObstaclePalette::FlameCore);
            builder.AddFacetedSphere(
                glm::vec3(0.0f, centerY + 0.085f, 0.0f),
                radius * 0.44f, 8, 5);

            // Two tongues licking up off the top. They cost two boxes and
            // are what stop the fireball reading as a plain orange ball.
            builder.SetColor(ObstaclePalette::Flame);

            for (int side = -1; side <= 1; side += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(side) * 0.085f, 0.0f,
                    0.07f, 0.07f,
                    centerY + 0.11f, centerY + 0.27f);
            }

            model.height = centerY + 0.27f;
            model.footprintWidth = radius * 2.2f;
            model.boundingRadius = radius;
            break;
        }

        case HazardType::Lightning:
        {
            // One mesh covering both phases of WarningThenStrike: a flat pad
            // marking the danger area on the ground, and the bolt standing in
            // it. MovingHazard dims the whole thing while it is only
            // telegraphing and brightens it on the strike, so the player gets
            // the warning the GDD asks for without a second model.
            builder.SetColor(ObstaclePalette::BoltEdge);
            builder.AddSlabAt(0.0f, 0.0f, 0.86f, 0.86f, 0.0f, 0.03f);

            // The bolt itself: stepped segments that alternate side to side,
            // so it reads as a zigzag rather than a post.
            builder.SetColor(ObstaclePalette::BoltCore);

            constexpr int Segments = 5;
            constexpr float SegmentHeight = 0.25f;

            for (int segment = 0; segment < Segments; ++segment)
            {
                const float bottom = 0.03f + SegmentHeight * segment;
                const float offset = (segment % 2 == 0) ? -0.055f : 0.055f;

                builder.AddSlabAt(
                    offset, 0.0f,
                    0.10f, 0.10f,
                    bottom, bottom + SegmentHeight);
            }

            model.height = 0.03f + SegmentHeight * Segments;
            model.footprintWidth = 0.86f;
            model.footprintDepth = 0.86f;
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
    // FORCE the correct mapping manually so it matches your new switch order!
    int index = 0;
    switch (type)
    {
    case ObstacleType::Tree:     index = 0; break;
    case ObstacleType::Rock:     index = 1; break;
    case ObstacleType::Fence:    index = 2; break;
    case ObstacleType::Wall:     index = 3; break;
    case ObstacleType::Bush:     index = 4; break;
    case ObstacleType::Spikes:   index = 5; break;
    case ObstacleType::Cow:      index = 6; break;
    case ObstacleType::Mud:      index = 7; break;
    case ObstacleType::Palisade: index = 8; break;
    default:                     index = 0; break;
    }

    ObstacleModel& model = obstacleModels[index];

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

    if (model.mesh)
    {
        hazard->Initialize();
    }
    else
    {
        // The behavior layer can still move this transform-only anchor, but
        // no quad or other placeholder visual is created for deferred types.
        hazard->SetShadowVisible(false);
    }

    return hazard;
}

void ObstacleMeshLibrary::Clear()
{
    for (ObstacleModel& model : obstacleModels)
        model = ObstacleModel();

    for (ObstacleModel& model : hazardModels)
        model = ObstacleModel();
}
