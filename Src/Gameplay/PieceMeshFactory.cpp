/*
    ============================================================
    Checkmate Crossing - Piece Mesh Factory

    Builds the human-like chess pieces out of boxes.

    Every figure shares one skeleton - feet, legs, torso, shoulders,
    arms, hands, head - and differs only in its headwear, its robe or
    legs, and the single prop it carries. That shared skeleton is what
    makes seven models read as one army.

    Proportions follow the reference sheet's own breakdown of the body
    into simple shapes, at roughly a 1 : 1.4 spread from the pawn to
    the king.
    ============================================================
*/

#include "PieceMeshFactory.h"

#include <algorithm>

#include "GameConfig.h"

namespace
{
    /// Maps figure space onto world axes.
    ///
    /// A figure facing the camera has its shoulders along X and its depth
    /// along Z. A rider facing down the lane has them swapped. Everything
    /// else in this file is written once, in figure space, and passes
    /// through here.
    glm::vec3 FigureVec(
        PieceMeshFactory::Facing facing,
        float across,
        float up,
        float forward)
    {
        return (facing == PieceMeshFactory::Facing::Camera)
            ? glm::vec3(across, up, forward)
            : glm::vec3(forward, up, across);
    }

    //---------------------------------------------------------
    // Horse layout, in world space.
    //
    // The horse is the one model that is not a humanoid, so it is laid out
    // directly along X rather than in figure space. It faces +X because the
    // camera looks straight down the lanes: a horse facing the camera would
    // be seen end-on and read as a shapeless lump, while one in profile
    // shows its whole outline.
    //---------------------------------------------------------

    constexpr float HorseHoofTop = 0.14f;
    constexpr float HorseLowerLegTop = 0.34f;
    constexpr float HorseBellyY = 0.44f;
    constexpr float HorseBackY = 0.70f;
    constexpr float HorseSaddleTop = 0.76f;
    constexpr float HorseEarTop = 0.97f;

    constexpr float HorseLegX = 0.19f;
    constexpr float HorseLegZ = 0.10f;
}

namespace PieceMeshFactory
{
    void AddFigurePart(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float across,
        float forward,
        float bottomY,
        float topY,
        float width,
        float depth)
    {
        const float height = topY - bottomY;

        if (height <= 0.0f || width <= 0.0f || depth <= 0.0f)
            return;

        builder.AddBox(
            FigureVec(spec.facing, across, bottomY + height * 0.5f, forward),
            FigureVec(spec.facing, width, height, depth));
    }

    //---------------------------------------------------------
    // Body parts
    //---------------------------------------------------------

    float AddLegs(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        // Both legs are built in one pass over the two sides, so they can
        // never drift out of step with each other.
        for (int side = -1; side <= 1; side += 2)
        {
            const float across = static_cast<float>(side) * spec.hipSpread;

            builder.SetColor(PiecePalette::Leather);

            // Feet reach forward a little, which is what stops a figure
            // looking like it is balanced on stilts.
            AddFigurePart(
                builder, spec, across, 0.02f,
                0.0f, spec.feetTop,
                spec.hipSpread * 1.6f, spec.torsoDepth * 0.95f);

            builder.SetColor(PiecePalette::Cloth);

            AddFigurePart(
                builder, spec, across, 0.0f,
                spec.feetTop, spec.kneeTop,
                spec.hipSpread * 1.35f, spec.torsoDepth * 0.68f);

            builder.SetColor(PiecePalette::Armor);

            AddFigurePart(
                builder, spec, across, 0.0f,
                spec.kneeTop, spec.hipTop,
                spec.hipSpread * 1.6f, spec.torsoDepth * 0.78f);
        }

        return spec.hipTop;
    }

    float AddRobe(
        MeshBuilder& builder,
        float bottomWidth,
        float topWidth,
        float topY)
    {
        builder.SetColor(PiecePalette::Cloth);

        // A frustum, not a stack of slabs: the robe needs genuinely sloped
        // sides or it turns into the ringed look the abstract set had.
        builder.AddFrustum(bottomWidth, topWidth, 0.0f, topY);

        return topY;
    }

    float AddTorso(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        builder.SetColor(PiecePalette::Armor);

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.hipTop, spec.torsoTop,
            spec.torsoWidth, spec.torsoDepth);

        builder.SetColor(PiecePalette::Leather);

        // The belt sits proud of the torso so it reads as a separate band
        // rather than a stripe.
        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.hipTop, spec.hipTop + 0.045f,
            spec.torsoWidth + 0.02f, spec.torsoDepth + 0.02f);

        builder.SetColor(PiecePalette::Armor);

        // Pauldrons: wider than the torso, which is what gives the figures
        // their armoured, top-heavy soldier shape.
        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.shoulderBottom, spec.shoulderTop,
            spec.shoulderWidth, spec.torsoDepth + 0.02f);

        return spec.shoulderTop;
    }

    float AddArms(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float forwardReach)
    {
        const float upperTop = spec.shoulderBottom + 0.02f;
        const float upperBottom = upperTop - 0.13f;
        const float lowerBottom = upperBottom - 0.13f;
        const float handBottom = lowerBottom - 0.06f;

        const float armWidth = spec.hipSpread * 1.2f;

        for (int side = -1; side <= 1; side += 2)
        {
            const float across = static_cast<float>(side) * spec.armSpread;

            builder.SetColor(PiecePalette::Armor);

            AddFigurePart(
                builder, spec, across, 0.0f,
                upperBottom, upperTop,
                armWidth, armWidth * 1.1f);

            builder.SetColor(PiecePalette::Cloth);

            // Forearms can angle forward, which is how the rider reaches the
            // reins and the bishop holds a book against his chest.
            AddFigurePart(
                builder, spec, across, forwardReach * 0.5f,
                lowerBottom, upperBottom + 0.01f,
                armWidth * 0.9f, armWidth);

            builder.SetColor(PiecePalette::Skin);

            AddFigurePart(
                builder, spec, across, forwardReach,
                handBottom, lowerBottom + 0.01f,
                armWidth, armWidth * 1.1f);
        }

        return handBottom;
    }

    float AddHead(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        builder.SetColor(PiecePalette::Skin);

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.shoulderTop, spec.headTop,
            spec.headWidth, spec.headDepth);

        return spec.headTop;
    }

    //---------------------------------------------------------
    // Headwear
    //---------------------------------------------------------

    float AddSoldierHelmet(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y)
    {
        builder.SetColor(PiecePalette::Armor);

        // The brim overlaps the top of the head so the helmet sits on it
        // rather than hovering above it.
        const float brimBottom = y - 0.035f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            brimBottom, y + 0.015f,
            spec.headWidth + 0.045f, spec.headDepth + 0.045f);

        const float domeTop = y + 0.09f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            y + 0.015f, domeTop,
            spec.headWidth + 0.01f, spec.headDepth + 0.01f);

        return domeTop;
    }

    float AddCastleHelmet(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y)
    {
        builder.SetColor(PiecePalette::Armor);

        // A straight-sided tower rather than a dome, so the battlements on
        // top have something to sit on.
        const float towerWidth = spec.headWidth + 0.055f;
        const float towerTop = y + 0.055f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            y - 0.045f, towerTop,
            towerWidth, spec.headDepth + 0.055f);

        return AddCrenellations(
            builder,
            towerWidth,
            towerWidth * 0.32f,
            towerTop,
            0.075f);
    }

    float AddMitre(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y)
    {
        builder.SetColor(PiecePalette::Metal);

        const float bandTop = y + 0.02f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            y - 0.03f, bandTop,
            spec.headWidth + 0.035f, spec.headDepth + 0.035f);

        builder.SetColor(PiecePalette::Armor);

        // The tall taper that makes a bishop unmistakable at a glance. It
        // stops at a small flat top rather than a spike.
        const float mitreTop = bandTop + 0.29f;

        builder.AddFrustum(
            spec.headWidth + 0.015f,
            0.06f,
            bandTop,
            mitreTop);

        return mitreTop;
    }

    float AddCrown(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y,
        float pointHeight,
        bool centrePoint,
        bool withCross)
    {
        builder.SetColor(PiecePalette::Metal);

        const float bandWidth = spec.headWidth + 0.03f;
        const float bandTop = y + 0.055f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            y - 0.02f, bandTop,
            bandWidth, spec.headDepth + 0.03f);

        // Tall points get proportionally thinner, which is what reads as a
        // royal crown rather than a castle wall.
        const float pointWidth = (pointHeight > 0.09f)
            ? bandWidth * 0.22f
            : bandWidth * 0.30f;

        float top = AddCrenellations(
            builder,
            bandWidth,
            pointWidth,
            bandTop,
            pointHeight);

        if (centrePoint)
        {
            // A taller fifth point in the middle, so the queen's crown is
            // unmistakably a crown from the front.
            builder.AddSlab(
                pointWidth * 1.15f,
                pointWidth * 1.15f,
                bandTop,
                bandTop + pointHeight * 1.3f);

            top = bandTop + pointHeight * 1.3f;
        }

        if (withCross)
            top = AddCross(builder, top, 0.18f, 0.05f, 0.15f);

        return top;
    }

    float AddCrenellations(
        MeshBuilder& builder,
        float ringWidth,
        float blockWidth,
        float y,
        float height)
    {
        const float offset = (ringWidth - blockWidth) * 0.5f;

        for (int ix = -1; ix <= 1; ix += 2)
        {
            for (int iz = -1; iz <= 1; iz += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(ix) * offset,
                    static_cast<float>(iz) * offset,
                    blockWidth,
                    blockWidth,
                    y,
                    y + height);
            }
        }

        return y + height;
    }

    float AddCross(
        MeshBuilder& builder,
        float y,
        float height,
        float barWidth,
        float armWidth)
    {
        builder.AddSlab(barWidth, barWidth, y, y + height);

        // The arm runs along X so it stays visible from the gameplay camera,
        // which only ever sees the front and top of a shape. Along Z it would
        // hide behind the upright and the cross would read as a plain post.
        const float armHeight = barWidth;
        const float armBottom = y + height * 0.34f;

        builder.AddSlab(
            armWidth,
            barWidth,
            armBottom,
            armBottom + armHeight);

        return y + height;
    }

    //---------------------------------------------------------
    // Held props
    //---------------------------------------------------------

    void AddSword(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY,
        float scale)
    {
        // Held point-down beside the figure, gripped where the hand already
        // is, so the two never come apart.
        const float across = spec.armSpread + 0.02f;

        const float gripTop = handY + 0.085f * scale;
        const float guardTop = handY - 0.01f;
        const float guardBottom = guardTop - 0.04f * scale;
        const float bladeBottom = 0.03f;

        builder.SetColor(PiecePalette::Leather);

        AddFigurePart(
            builder, spec, across, 0.0f,
            handY - 0.01f, gripTop,
            0.05f * scale, 0.05f * scale);

        builder.SetColor(PiecePalette::Metal);

        // Pommel, so the grip does not end in mid-air.
        AddFigurePart(
            builder, spec, across, 0.0f,
            gripTop, gripTop + 0.035f * scale,
            0.065f * scale, 0.065f * scale);

        AddFigurePart(
            builder, spec, across, 0.0f,
            guardBottom, guardTop,
            0.15f * scale, 0.055f * scale);

        AddFigurePart(
            builder, spec, across, 0.0f,
            bladeBottom, guardBottom + 0.005f,
            0.075f * scale, 0.045f * scale);
    }

    void AddShield(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY)
    {
        // Carried on the figure's other side from where a sword would be, and
        // pushed forward so it faces the camera.
        //
        // It has to run from hip to shoulder to read as a shield. Sized to
        // the hand alone it sat down by the knee and looked like a crate on
        // the ground next to him.
        const float across = -(spec.armSpread + 0.06f);

        const float bottomY = handY - 0.10f;
        const float topY = handY + 0.36f;

        builder.SetColor(PiecePalette::Metal);

        AddFigurePart(
            builder, spec, across, 0.10f,
            bottomY, topY,
            0.26f, 0.05f);

        // A darker device inset in the face, so the shield is not one flat
        // rectangle. It has to be darker than the rim: as a lighter boss it
        // vanished into the plate and the whole thing read as a plain door.
        builder.SetColor(PiecePalette::Cloth);

        AddFigurePart(
            builder, spec, across, 0.135f,
            bottomY + 0.06f, topY - 0.06f,
            0.17f, 0.03f);
    }

    void AddBook(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY)
    {
        // Resting on the open hand and held up against the chest, which is
        // what separates the bishop's silhouette from the queen's.
        //
        // Pushed well clear of the torso on both axes: tucked in tight it
        // disappeared into the chest and the bishop lost the one prop that
        // identifies him.
        const float across = spec.armSpread + 0.05f;

        const float bottomY = handY + 0.01f;
        const float topY = bottomY + 0.17f;

        builder.SetColor(PiecePalette::Leather);

        AddFigurePart(
            builder, spec, across, 0.145f,
            bottomY, topY,
            0.19f, 0.07f);

        builder.SetColor(PiecePalette::Armor);

        // Pages, showing as a bright edge along the front of the cover.
        AddFigurePart(
            builder, spec, across, 0.18f,
            bottomY + 0.02f, topY - 0.02f,
            0.155f, 0.03f);
    }

    float AddStaff(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY,
        float topY)
    {
        const float across = spec.armSpread + 0.025f;

        const float headBottom = topY - 0.09f;

        builder.SetColor(PiecePalette::Wood);

        AddFigurePart(
            builder, spec, across, 0.0f,
            handY - 0.14f, headBottom,
            0.042f, 0.042f);

        builder.SetColor(PiecePalette::Metal);

        AddFigurePart(
            builder, spec, across, 0.0f,
            headBottom, topY,
            0.095f, 0.095f);

        return topY;
    }

    void AddCape(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY)
    {
        builder.SetColor(PiecePalette::Cloth);

        const float back = -(spec.torsoDepth * 0.5f + 0.035f);
        const float midY = bottomY + (spec.shoulderTop - bottomY) * 0.45f;

        // Two slabs rather than one, so the cape flares towards the hem
        // instead of hanging as a plain rectangle.
        AddFigurePart(
            builder, spec, 0.0f, back,
            midY, spec.shoulderTop,
            spec.shoulderWidth * 0.88f, 0.05f);

        AddFigurePart(
            builder, spec, 0.0f, back,
            bottomY, midY + 0.01f,
            spec.shoulderWidth, 0.05f);
    }

    void AddHair(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY,
        float topY)
    {
        builder.SetColor(PiecePalette::Hair);

        // One slab behind the head and shoulders. Long hair is the queen's
        // clearest read from behind and from the side.
        AddFigurePart(
            builder, spec, 0.0f, -(spec.headDepth * 0.5f + 0.02f),
            bottomY, topY,
            spec.headWidth + 0.03f, 0.06f);
    }

    void AddBeard(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY,
        float topY)
    {
        builder.SetColor(PiecePalette::Hair);

        AddFigurePart(
            builder, spec, 0.0f, spec.headDepth * 0.5f + 0.005f,
            bottomY, topY,
            spec.headWidth * 0.82f, 0.06f);
    }

    //---------------------------------------------------------
    // Horse
    //---------------------------------------------------------

    float AddHorse(MeshBuilder& builder)
    {
        builder.SetColor(PiecePalette::Leather);

        // Broad hooves anchor four slimmer lower legs.
        for (int ix = -1; ix <= 1; ix += 2)
        {
            for (int iz = -1; iz <= 1; iz += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(ix) * HorseLegX,
                    static_cast<float>(iz) * HorseLegZ,
                    0.14f, 0.145f,
                    0.0f, HorseHoofTop);
            }
        }

        builder.SetColor(PiecePalette::Armor);

        // Lower legs stay narrow; short upper legs broaden into the body.
        for (int ix = -1; ix <= 1; ix += 2)
        {
            for (int iz = -1; iz <= 1; iz += 2)
            {
                builder.AddSlabAt(
                    static_cast<float>(ix) * HorseLegX,
                    static_cast<float>(iz) * HorseLegZ,
                    0.085f, 0.09f,
                    HorseHoofTop, HorseLowerLegTop);

                builder.AddSlabAt(
                    static_cast<float>(ix) * HorseLegX,
                    static_cast<float>(iz) * HorseLegZ,
                    0.115f, 0.11f,
                    HorseLowerLegTop - 0.01f, HorseBellyY + 0.03f);
            }
        }

        // Overlapping rump, barrel and chest masses replace the rectangular
        // torso while retaining one clean, blocky silhouette.
        builder.AddSlabAt(
            -0.20f, 0.0f,
            0.20f, 0.29f,
            HorseBellyY + 0.04f, HorseBackY - 0.01f);

        builder.AddSlabAt(
            -0.03f, 0.0f,
            0.38f, 0.28f,
            HorseBellyY + 0.02f, HorseBackY - 0.02f);

        builder.AddSlabAt(
            0.17f, 0.0f,
            0.20f, 0.30f,
            HorseBellyY + 0.01f, HorseBackY);

        // Three narrowing, overlapping blocks climb from the chest to the
        // poll. Their stepped outline reads as an angled neck from gameplay
        // distance without adding a new rotated-primitive system.
        builder.AddSlabAt(
            0.22f, 0.0f,
            0.18f, 0.24f,
            0.62f, 0.78f);

        builder.AddSlabAt(
            0.28f, 0.0f,
            0.16f, 0.21f,
            0.70f, 0.85f);

        builder.AddSlabAt(
            0.34f, 0.0f,
            0.14f, 0.18f,
            0.78f, 0.88f);

        // A compact cranium, cheek and descending nose establish the horse
        // profile. The nose keeps the old forward limit for border clearance.
        builder.AddSlabAt(
            0.39f, 0.0f,
            0.16f, 0.17f,
            0.80f, 0.90f);

        builder.AddSlabAt(
            0.43f, 0.0f,
            0.13f, 0.16f,
            0.75f, 0.83f);

        builder.AddSlabAt(
            0.48f, 0.0f,
            0.15f, 0.135f,
            0.755f, 0.83f);

        builder.AddSlabAt(
            0.535f, 0.0f,
            0.08f, 0.13f,
            0.71f, 0.77f);

        // A small lower jaw separates the snout from the throat.
        builder.AddSlabAt(
            0.47f, 0.0f,
            0.14f, 0.12f,
            0.69f, 0.735f);

        // The ears are separated across the head so both survive a 3/4 view.
        for (int iz = -1; iz <= 1; iz += 2)
        {
            builder.AddSlabAt(
                0.35f,
                static_cast<float>(iz) * 0.052f,
                0.05f, 0.045f,
                0.885f, HorseEarTop);
        }

        builder.SetColor(PiecePalette::Mane);

        // A stepped mane reinforces the rising rear edge of the neck.
        builder.AddSlabAt(
            0.105f, 0.0f,
            0.06f, 0.17f,
            0.63f, 0.77f);

        builder.AddSlabAt(
            0.18f, 0.0f,
            0.06f, 0.16f,
            0.73f, 0.86f);

        builder.AddSlabAt(
            0.26f, 0.0f,
            0.06f, 0.15f,
            0.81f, 0.90f);

        // Three descending blocks give the rump a clear tail silhouette.
        builder.AddSlabAt(
            -0.315f, 0.0f,
            0.07f, 0.10f,
            0.54f, 0.68f);

        builder.AddSlabAt(
            -0.34f, 0.0f,
            0.065f, 0.08f,
            0.40f, 0.56f);

        builder.AddSlabAt(
            -0.345f, 0.0f,
            0.055f, 0.07f,
            0.28f, 0.43f);

        builder.SetColor(PiecePalette::Cloth);

        // Caparison hanging over the horse's flanks, below the saddle.
        builder.AddSlabAt(
            0.02f, 0.0f,
            0.30f, 0.345f,
            0.60f, HorseBackY + 0.01f);

        builder.SetColor(PiecePalette::Leather);

        builder.AddSlabAt(
            0.03f, 0.0f,
            0.21f, 0.27f,
            HorseBackY, HorseSaddleTop);

        return HorseSaddleTop;
    }

    float GetHorseTopY()
    {
        return HorseEarTop;
    }

    //---------------------------------------------------------
    // Piece models
    //---------------------------------------------------------

    PieceModel Create(PieceType type)
    {
        MeshBuilder builder;

        PieceModel model;

        // Standing figures share one footprint, which keeps their shadows
        // consistent. The horse overrides it.
        model.baseWidth = 0.36f;
        model.baseDepth = 0.30f;

        switch (type)
        {
        case PieceType::Pawn:
        {
            // The baseline soldier: plain helmet, small sword. Every other
            // humanoid is this figure with different headwear and props.
            FigureSpec spec;

            AddLegs(builder, spec);
            AddTorso(builder, spec);

            const float handY = AddArms(builder, spec);

            const float headTop = AddHead(builder, spec);

            model.height = AddSoldierHelmet(builder, spec, headTop);

            AddSword(builder, spec, handY, 1.0f);
            break;
        }

        case PieceType::Rook:
        {
            // Same soldier, broader across the shoulders, with a battlemented
            // helmet and a shield instead of a sword.
            FigureSpec spec;
            spec.shoulderWidth = 0.43f;
            spec.torsoWidth = 0.30f;

            AddLegs(builder, spec);
            AddTorso(builder, spec);

            const float handY = AddArms(builder, spec);

            const float headTop = AddHead(builder, spec);

            model.height = AddCastleHelmet(builder, spec, headTop);

            AddShield(builder, spec, handY);
            break;
        }

        case PieceType::Knight:
        {
            // The horse alone, saddled. The only piece that is not a figure.
            AddHorse(builder);

            model.height = GetHorseTopY();
            model.baseWidth = 0.62f;
            model.baseDepth = 0.34f;
            break;
        }

        case PieceType::MountedPawn:
        {
            // The same horse and the same soldier, with the figure turned to
            // face down the lane and its legs straddling the saddle. Nothing
            // here is a new shape - only a new pose.
            const float saddleY = AddHorse(builder);

            FigureSpec spec;
            spec.facing = Facing::AlongX;

            // Lift the whole figure onto the saddle. Its landmarks stay in
            // the same order and the same proportions.
            const float lift = saddleY - 0.05f;

            spec.hipTop = lift + 0.09f;
            spec.torsoTop = lift + 0.30f;
            spec.shoulderBottom = lift + 0.24f;
            spec.shoulderTop = lift + 0.34f;
            spec.headTop = lift + 0.48f;

            // Thighs across the saddle and shins hanging down its sides,
            // instead of the standing leg stack.
            for (int side = -1; side <= 1; side += 2)
            {
                const float across =
                    static_cast<float>(side) * (spec.hipSpread + 0.09f);

                builder.SetColor(PiecePalette::Armor);

                AddFigurePart(
                    builder, spec, across, 0.05f,
                    lift + 0.01f, spec.hipTop + 0.02f,
                    0.115f, 0.20f);

                builder.SetColor(PiecePalette::Cloth);

                AddFigurePart(
                    builder, spec, across, 0.125f,
                    lift - 0.20f, lift + 0.03f,
                    0.105f, 0.115f);

                builder.SetColor(PiecePalette::Leather);

                AddFigurePart(
                    builder, spec, across, 0.15f,
                    lift - 0.26f, lift - 0.19f,
                    0.115f, 0.15f);
            }

            AddTorso(builder, spec);

            // Forearms angled forward to the reins.
            AddArms(builder, spec, 0.09f);

            const float headTop = AddHead(builder, spec);

            model.height = AddSoldierHelmet(builder, spec, headTop);

            model.baseWidth = 0.62f;
            model.baseDepth = 0.34f;
            break;
        }

        case PieceType::Bishop:
        {
            // A robe instead of legs, a tall mitre, and a book held up
            // against the chest.
            FigureSpec spec;
            spec.shoulderWidth = 0.38f;
            spec.hipTop = 0.54f;
            spec.torsoTop = 0.74f;
            spec.shoulderBottom = 0.68f;
            spec.shoulderTop = 0.77f;
            spec.headTop = 0.91f;

            AddRobe(builder, 0.42f, 0.28f, spec.hipTop);
            AddTorso(builder, spec);

            const float handY = AddArms(builder, spec, 0.04f);

            const float headTop = AddHead(builder, spec);

            model.height = AddMitre(builder, spec, headTop);

            AddBook(builder, spec, handY);

            model.baseWidth = 0.42f;
            model.baseDepth = 0.42f;
            break;
        }

        case PieceType::Queen:
        {
            // Robe, long hair, a pointed crown, and a scepter that stands
            // taller than the crown itself.
            FigureSpec spec;
            spec.shoulderWidth = 0.36f;
            spec.torsoWidth = 0.27f;
            spec.armSpread = 0.175f;
            spec.headWidth = 0.18f;
            spec.headDepth = 0.16f;
            spec.hipTop = 0.56f;
            spec.torsoTop = 0.76f;
            spec.shoulderBottom = 0.70f;
            spec.shoulderTop = 0.79f;
            spec.headTop = 0.93f;

            AddRobe(builder, 0.44f, 0.28f, spec.hipTop);
            AddTorso(builder, spec);

            const float handY = AddArms(builder, spec);

            AddHair(builder, spec, 0.70f, spec.headTop + 0.03f);

            const float headTop = AddHead(builder, spec);

            // Tall thin points plus a taller centre one.
            AddCrown(builder, spec, headTop, 0.11f, true, false);

            // The scepter is the tallest thing she carries, so it sets her
            // reported height.
            model.height = AddStaff(builder, spec, handY, 1.20f);

            model.baseWidth = 0.44f;
            model.baseDepth = 0.44f;
            break;
        }

        case PieceType::King:
        {
            // The bulkiest figure: broader torso and shoulders, a cape down
            // his back, a beard, a crown topped with a cross, and a sword
            // half again the pawn's.
            FigureSpec spec;
            spec.shoulderWidth = 0.45f;
            spec.torsoWidth = 0.32f;
            spec.torsoDepth = 0.19f;
            spec.hipSpread = 0.085f;
            spec.armSpread = 0.20f;
            spec.headWidth = 0.20f;
            spec.headDepth = 0.18f;
            spec.feetTop = 0.08f;
            spec.kneeTop = 0.27f;
            spec.hipTop = 0.46f;
            spec.torsoTop = 0.72f;
            spec.shoulderBottom = 0.65f;
            spec.shoulderTop = 0.76f;
            spec.headTop = 0.91f;

            AddLegs(builder, spec);

            // Cape before the torso, so the torso is what wins where they
            // overlap at the shoulders.
            AddCape(builder, spec, 0.12f);

            AddTorso(builder, spec);

            const float handY = AddArms(builder, spec);

            // Over the lower half of the face, not below it. Sitting any
            // lower it stopped reading as a beard and became a dark bib
            // across his chest.
            AddBeard(builder, spec, 0.77f, 0.855f);

            const float headTop = AddHead(builder, spec);

            // Short thick points under the cross.
            model.height =
                AddCrown(builder, spec, headTop, 0.075f, false, true);

            AddSword(builder, spec, handY, 1.45f);

            model.baseWidth = 0.40f;
            model.baseDepth = 0.32f;
            break;
        }
        }

        model.mesh = builder.Build();

        return model;
    }
}

PieceMeshLibrary::PieceMeshLibrary()
{
}

const PieceModel& PieceMeshLibrary::GetModel(PieceType type)
{
    const int index = static_cast<int>(type);

    PieceModel& model = models[index];

    if (!model.mesh)
        model = PieceMeshFactory::Create(type);

    return model;
}

std::shared_ptr<ChessPiece> PieceMeshLibrary::CreatePiece(
    PieceType type,
    PieceTeam team)
{
    const PieceModel& model = GetModel(type);

    auto piece = std::make_shared<ChessPiece>(type, team);

    piece->SetMesh(model.mesh);

    // Scaling through the transform rather than the geometry means every
    // piece of a type still shares one mesh. The pivot is under the figure's
    // feet, so scaling about the origin leaves it standing on the ground.
    piece->GetTransform().SetScale(
        GameConfig::PieceScale,
        GameConfig::PieceScale,
        GameConfig::PieceScale);

    piece->SetDimensions(
        model.height * GameConfig::PieceScale,
        model.baseWidth * GameConfig::PieceScale,
        model.baseDepth * GameConfig::PieceScale);

    piece->Initialize();

    return piece;
}

void PieceMeshLibrary::Clear()
{
    for (PieceModel& model : models)
        model = PieceModel();
}
