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
#include "PieceRig.h"

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
    // Horse layout, in the animal's own frame.
    //
    // The horse is the one model that is not a humanoid, so its parts are
    // measured along its body rather than in figure space: "along" runs from
    // tail to nose and "across" from flank to flank. HorseVec and
    // AddHorseSlab are the only two places that turn those into world axes.
    //
    // It used to be written straight into world space with its body along X,
    // which made it the single model in the set whose forward was not +Z.
    // Nothing had ever given a horse a heading, so the mismatch stayed
    // invisible - but any piece that turned would have pointed ninety
    // degrees away from a pawn given the same heading.
    //---------------------------------------------------------

    constexpr float HorseHoofTop = 0.14f;
    constexpr float HorseLowerLegTop = 0.34f;
    constexpr float HorseBellyY = 0.44f;
    constexpr float HorseBackY = 0.70f;
    constexpr float HorseSaddleTop = 0.76f;
    constexpr float HorseEarTop = 0.97f;

    /// Half the gap between the front and rear legs, and between the left
    /// and right pair.
    constexpr float HorseLegAlong = 0.19f;
    constexpr float HorseLegAcross = 0.10f;

    /// Maps the horse's own frame onto world axes.
    ///
    /// The counterpart of FigureVec, and the reason normalising the horse
    /// was a change of convention rather than of shape.
    glm::vec3 HorseVec(float along, float up, float across)
    {
        return glm::vec3(across, up, along);
    }
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

    float GetHipPivotY(const FigureSpec& spec)
    {
        return spec.hipTop;
    }

    float GetShoulderPivotY(const FigureSpec& spec)
    {
        // Where the upper arm meets the pauldron, which is the point it has
        // to swing about. Duplicated from AddArm's own first line on purpose:
        // both read it from the spec, so neither can define it privately.
        return spec.shoulderBottom + 0.02f;
    }

    float GetHandY(const FigureSpec& spec)
    {
        return GetShoulderPivotY(spec) - 0.13f - 0.13f - 0.06f;
    }

    void AddLeg(
        MeshBuilder& builder,
        const FigureSpec& spec,
        int side)
    {
        const float across = static_cast<float>(side) * spec.hipSpread;

        builder.SetColor(spec.materials.boots);

        // Feet reach forward a little, which is what stops a figure
        // looking like it is balanced on stilts.
        AddFigurePart(
            builder, spec, across, 0.02f,
            0.0f, spec.feetTop,
            spec.hipSpread * 1.6f, spec.torsoDepth * 0.95f);

        // Shin: trousers under the knee, not plate.
        builder.SetColor(spec.materials.clothBlue);

        AddFigurePart(
            builder, spec, across, 0.0f,
            spec.feetTop, spec.kneeTop,
            spec.hipSpread * 1.35f, spec.torsoDepth * 0.68f);

        // Thigh: the armoured half of the leg.
        builder.SetColor(spec.materials.armorLight);

        AddFigurePart(
            builder, spec, across, 0.0f,
            spec.kneeTop, spec.hipTop,
            spec.hipSpread * 1.6f, spec.torsoDepth * 0.78f);
    }

    float AddLegs(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        // Both legs come from one pass over the two sides, so they can never
        // drift out of step - and both come from the same AddLeg the rig
        // uses, so the baked figure and the animated one are the same shape.
        for (int side = -1; side <= 1; side += 2)
            AddLeg(builder, spec, side);

        return spec.hipTop;
    }

    float AddRobe(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomWidth,
        float topWidth,
        float topY)
    {
        builder.SetColor(spec.materials.clothBlue);

        // A frustum, not a stack of slabs: the robe needs genuinely sloped
        // sides or it turns into the ringed look the abstract set had.
        builder.AddFrustum(bottomWidth, topWidth, 0.0f, topY);

        return topY;
    }

    float AddTorso(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        builder.SetColor(spec.materials.armorLight);

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.hipTop, spec.torsoTop,
            spec.torsoWidth, spec.torsoDepth);

        if (spec.detailedFace)
        {
            // A band of the undershirt showing at the neckline, which is
            // what the reference uses to break up an otherwise solid
            // breastplate. Inset on both axes so it sits inside the torso
            // rather than growing it.
            builder.SetColor(spec.materials.clothBlue);

            AddFigurePart(
                builder, spec, 0.0f, spec.torsoDepth * 0.5f - 0.005f,
                spec.torsoTop - 0.055f, spec.torsoTop + 0.005f,
                spec.torsoWidth * 0.66f, 0.02f);
        }

        builder.SetColor(spec.materials.leather);

        // The belt sits proud of the torso so it reads as a separate band
        // rather than a stripe.
        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.hipTop, spec.hipTop + 0.045f,
            spec.torsoWidth + 0.02f, spec.torsoDepth + 0.02f);

        if (spec.detailedFace)
        {
            // Buckle, proud of the belt on the front face only.
            builder.SetColor(spec.materials.gold);

            AddFigurePart(
                builder, spec, 0.0f, spec.torsoDepth * 0.5f + 0.014f,
                spec.hipTop + 0.004f, spec.hipTop + 0.041f,
                0.05f, 0.018f);
        }

        builder.SetColor(spec.materials.armorLight);

        // Pauldrons: wider than the torso, which is what gives the figures
        // their armoured, top-heavy soldier shape.
        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.shoulderBottom, spec.shoulderTop,
            spec.shoulderWidth, spec.torsoDepth + 0.02f);

        if (spec.detailedFace)
        {
            // The outer third of each pauldron dropped a shade, so the
            // shoulders read as two caps over a chest rather than one bar
            // laid across it.
            builder.SetColor(spec.materials.armorShadow);

            for (int side = -1; side <= 1; side += 2)
            {
                AddFigurePart(
                    builder, spec,
                    static_cast<float>(side) * spec.shoulderWidth * 0.375f,
                    0.0f,
                    spec.shoulderBottom - 0.012f, spec.shoulderTop,
                    spec.shoulderWidth * 0.25f, spec.torsoDepth + 0.03f);
            }
        }

        return spec.shoulderTop;
    }

    void AddArm(
        MeshBuilder& builder,
        const FigureSpec& spec,
        int side,
        float forwardReach)
    {
        const float upperTop = GetShoulderPivotY(spec);
        const float upperBottom = upperTop - 0.13f;
        const float lowerBottom = upperBottom - 0.13f;
        const float handBottom = lowerBottom - 0.06f;

        const float armWidth = spec.hipSpread * 1.2f;

        const float across = static_cast<float>(side) * spec.armSpread;

        builder.SetColor(spec.materials.armorLight);

        AddFigurePart(
            builder, spec, across, 0.0f,
            upperBottom, upperTop,
            armWidth, armWidth * 1.1f);

        // Sleeve below the pauldron, so the arm is not one solid bar of
        // plate from shoulder to wrist.
        builder.SetColor(spec.materials.clothBlue);

        // Forearms can angle forward, which is how the rider reaches the
        // reins and the bishop holds a book against his chest.
        AddFigurePart(
            builder, spec, across, forwardReach * 0.5f,
            lowerBottom, upperBottom + 0.01f,
            armWidth * 0.9f, armWidth);

        builder.SetColor(spec.materials.skin);

        AddFigurePart(
            builder, spec, across, forwardReach,
            handBottom, lowerBottom + 0.01f,
            armWidth, armWidth * 1.1f);
    }

    float AddArms(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float forwardReach)
    {
        for (int side = -1; side <= 1; side += 2)
            AddArm(builder, spec, side, forwardReach);

        return GetHandY(spec);
    }

    float AddHead(
        MeshBuilder& builder,
        const FigureSpec& spec)
    {
        builder.SetColor(spec.materials.skin);

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            spec.shoulderTop, spec.headTop,
            spec.headWidth, spec.headDepth);

        if (spec.detailedFace)
        {
            const float faceZ = spec.headDepth * 0.5f;
            const float eyeY = spec.headTop - (spec.headTop - spec.shoulderTop) * 0.42f;

            // Hair down both sides of the head and across the back, framing
            // the face the way the reference sheet's head study does. Left
            // as three thin slabs rather than a cap, because a helmet or a
            // mitre covers the crown on every figure that has one.
            builder.SetColor(spec.materials.hair);

            for (int side = -1; side <= 1; side += 2)
            {
                AddFigurePart(
                    builder, spec,
                    static_cast<float>(side) * (spec.headWidth * 0.5f + 0.008f),
                    0.0f,
                    eyeY - 0.035f, spec.headTop,
                    0.022f, spec.headDepth * 0.92f);
            }

            AddFigurePart(
                builder, spec, 0.0f, -(faceZ + 0.008f),
                eyeY - 0.035f, spec.headTop,
                spec.headWidth, 0.022f);

            // Two small blocks standing just proud of the face. At this
            // scale they are the whole difference between a head and a
            // thumb - without them the figure reads as faceless from the
            // gameplay camera.
            builder.SetColor(spec.materials.eyes);

            for (int side = -1; side <= 1; side += 2)
            {
                AddFigurePart(
                    builder, spec,
                    static_cast<float>(side) * spec.headWidth * 0.22f,
                    faceZ + 0.006f,
                    eyeY, eyeY + 0.028f,
                    0.030f, 0.014f);
            }
        }

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
        builder.SetColor(spec.materials.armorLight);

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
        builder.SetColor(spec.materials.armorLight);

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
        builder.SetColor(spec.materials.gold);

        const float bandTop = y + 0.02f;

        AddFigurePart(
            builder, spec, 0.0f, 0.0f,
            y - 0.03f, bandTop,
            spec.headWidth + 0.035f, spec.headDepth + 0.035f);

        builder.SetColor(spec.materials.armorLight);

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
        builder.SetColor(spec.materials.gold);

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

        builder.SetColor(spec.materials.leather);

        AddFigurePart(
            builder, spec, across, 0.0f,
            handY - 0.01f, gripTop,
            0.05f * scale, 0.05f * scale);

        builder.SetColor(spec.materials.gold);

        // Pommel, so the grip does not end in mid-air.
        AddFigurePart(
            builder, spec, across, 0.0f,
            gripTop, gripTop + 0.035f * scale,
            0.065f * scale, 0.065f * scale);

        AddFigurePart(
            builder, spec, across, 0.0f,
            guardBottom, guardTop,
            0.15f * scale, 0.055f * scale);

        // Steel, not brass. In the legacy set blade and gold are the same
        // value, so the King's sword is untouched by the split.
        builder.SetColor(spec.materials.blade);

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

        builder.SetColor(spec.materials.blade);

        AddFigurePart(
            builder, spec, across, 0.10f,
            bottomY, topY,
            0.26f, 0.05f);

        // A darker device inset in the face, so the shield is not one flat
        // rectangle. It has to be darker than the rim: as a lighter boss it
        // vanished into the plate and the whole thing read as a plain door.
        builder.SetColor(spec.materials.clothBlue);

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

        builder.SetColor(spec.materials.leather);

        AddFigurePart(
            builder, spec, across, 0.145f,
            bottomY, topY,
            0.19f, 0.07f);

        builder.SetColor(spec.materials.armorLight);

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

        builder.SetColor(spec.materials.wood);

        AddFigurePart(
            builder, spec, across, 0.0f,
            handY - 0.14f, headBottom,
            0.042f, 0.042f);

        builder.SetColor(spec.materials.blade);

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
        builder.SetColor(spec.materials.clothBlue);

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
        builder.SetColor(spec.materials.hair);

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
        builder.SetColor(spec.materials.hair);

        AddFigurePart(
            builder, spec, 0.0f, spec.headDepth * 0.5f + 0.005f,
            bottomY, topY,
            spec.headWidth * 0.82f, 0.06f);
    }

    //---------------------------------------------------------
    // Horse
    //---------------------------------------------------------

    /// Whether a horse painted with these materials gets its tack.
    ///
    /// Tack - the bridle, the reins and the brass - is added only when the
    /// coat and the muzzle are actually different colours. The mounted pawn
    /// shares these builders and is out of scope, and it is painted from the
    /// legacy set, where the two are equal.
    /// One block of the horse, given in the animal's own frame.
    ///
    /// "along" runs from tail to nose and "across" from flank to flank, and
    /// this is the single place that decides which world axis each of those
    /// lands on. The horse used to be written straight into world space with
    /// its body along X, which left it as the one model in the set facing a
    /// different way from every other - so a heading that pointed a pawn
    /// north pointed a horse east.
    ///
    /// Everything now faces +Z. Routing the horse through one mapping rather
    /// than renaming thirty coordinates by hand is what makes that a change
    /// of convention rather than a change of shape.
    void AddHorseSlab(
        MeshBuilder& builder,
        float along,
        float across,
        float length,
        float width,
        float bottomY,
        float topY)
    {
        builder.AddSlabAt(across, along, width, length, bottomY, topY);
    }

    bool HorseIsDetailed(const PieceMaterials& materials)
    {
        return materials.coat != materials.coatShadow;
    }

    void AddHorseLeg(
        MeshBuilder& builder,
        const PieceMaterials& materials,
        int ix,
        int iz)
    {
        const float x = static_cast<float>(ix) * HorseLegAlong;
        const float z = static_cast<float>(iz) * HorseLegAcross;

        builder.SetColor(materials.leatherDark);

        // A broad hoof anchors a slimmer lower leg.
        AddHorseSlab(builder, x, z, 0.14f, 0.145f, 0.0f, HorseHoofTop);

        builder.SetColor(materials.coat);

        // Lower leg stays narrow; the short upper leg broadens into the body.
        AddHorseSlab(builder, 
            x, z, 0.085f, 0.09f,
            HorseHoofTop, HorseLowerLegTop);

        AddHorseSlab(builder, 
            x, z, 0.115f, 0.11f,
            HorseLowerLegTop - 0.01f, HorseBellyY + 0.03f);
    }

    void AddHorseTail(
        MeshBuilder& builder,
        const PieceMaterials& materials)
    {
        builder.SetColor(materials.mane);

        // Three descending blocks give the rump a clear tail silhouette.
        AddHorseSlab(builder, -0.315f, 0.0f, 0.07f, 0.10f, 0.54f, 0.68f);
        AddHorseSlab(builder, -0.34f, 0.0f, 0.065f, 0.08f, 0.40f, 0.56f);
        AddHorseSlab(builder, -0.345f, 0.0f, 0.055f, 0.07f, 0.28f, 0.43f);
    }

    void AddHorseBody(
        MeshBuilder& builder,
        const PieceMaterials& materials)
    {
        const bool detailed = HorseIsDetailed(materials);

        builder.SetColor(materials.coat);

        // Overlapping rump, barrel and chest masses replace the rectangular
        // torso while retaining one clean, blocky silhouette.
        AddHorseSlab(builder, 
            -0.20f, 0.0f,
            0.20f, 0.29f,
            HorseBellyY + 0.04f, HorseBackY - 0.01f);

        AddHorseSlab(builder, 
            -0.03f, 0.0f,
            0.38f, 0.28f,
            HorseBellyY + 0.02f, HorseBackY - 0.02f);

        AddHorseSlab(builder, 
            0.17f, 0.0f,
            0.20f, 0.30f,
            HorseBellyY + 0.01f, HorseBackY);

        builder.SetColor(materials.clothBlue);

        // Caparison hanging over the horse's flanks, below the saddle.
        AddHorseSlab(builder, 
            0.02f, 0.0f,
            0.30f, 0.345f,
            0.60f, HorseBackY + 0.01f);

        builder.SetColor(materials.leather);

        AddHorseSlab(builder, 
            0.03f, 0.0f,
            0.21f, 0.27f,
            HorseBackY, HorseSaddleTop);

        if (detailed)
        {
            // Girth strap round the barrel, holding the saddle on.
            AddHorseSlab(builder, 
                0.03f, 0.0f,
                0.05f, 0.30f,
                HorseBellyY, HorseBackY + 0.005f);

            // Brass on each side of the breast strap.
            builder.SetColor(materials.gold);

            for (int iz = -1; iz <= 1; iz += 2)
            {
                AddHorseSlab(builder, 
                    0.30f,
                    static_cast<float>(iz) * 0.10f,
                    0.05f, 0.045f,
                    0.615f, 0.66f);
            }
        }
    }

    float AddHorse(
        MeshBuilder& builder,
        const PieceMaterials& materials)
    {
        // The whole animal, in the order the rig splits it. The static model
        // and the animated one are the same four calls; the rig only sends
        // each into its own mesh with its own joint at the origin.
        for (int ix = -1; ix <= 1; ix += 2)
        {
            for (int iz = -1; iz <= 1; iz += 2)
                AddHorseLeg(builder, materials, ix, iz);
        }

        AddHorseBody(builder, materials);
        AddHorseNeckHead(builder, materials);

        builder.SetColor(materials.coat);

        AddHorseTail(builder, materials);

        return HorseSaddleTop;
    }

    void AddHorseNeckHead(
        MeshBuilder& builder,
        const PieceMaterials& materials)
    {
        const bool detailed = HorseIsDetailed(materials);

        builder.SetColor(materials.coat);

        // Three narrowing, overlapping blocks climb from the chest to the
        // poll. Their stepped outline reads as an angled neck from gameplay
        // distance without adding a new rotated-primitive system.
        AddHorseSlab(builder, 
            0.22f, 0.0f,
            0.18f, 0.24f,
            0.62f, 0.78f);

        AddHorseSlab(builder, 
            0.28f, 0.0f,
            0.16f, 0.21f,
            0.70f, 0.85f);

        AddHorseSlab(builder, 
            0.34f, 0.0f,
            0.14f, 0.18f,
            0.78f, 0.88f);

        // A compact cranium, cheek and descending nose establish the horse
        // profile. The nose keeps the old forward limit for border clearance.
        AddHorseSlab(builder, 
            0.39f, 0.0f,
            0.16f, 0.17f,
            0.80f, 0.90f);

        AddHorseSlab(builder, 
            0.43f, 0.0f,
            0.13f, 0.16f,
            0.75f, 0.83f);

        // Muzzle: the snout, nose and jaw drop to the darker coat tone, the
        // way a grey horse's nose does. In the compatibility set the two
        // tones are the same value, preserving its flatter treatment.
        builder.SetColor(materials.coatShadow);

        AddHorseSlab(builder, 
            0.48f, 0.0f,
            0.15f, 0.135f,
            0.755f, 0.83f);

        AddHorseSlab(builder, 
            0.535f, 0.0f,
            0.08f, 0.13f,
            0.71f, 0.77f);

        // A small lower jaw separates the snout from the throat.
        AddHorseSlab(builder, 
            0.47f, 0.0f,
            0.14f, 0.12f,
            0.69f, 0.735f);

        builder.SetColor(materials.coat);

        // The ears are separated across the head so both survive a 3/4 view.
        for (int iz = -1; iz <= 1; iz += 2)
        {
            AddHorseSlab(builder, 
                0.35f,
                static_cast<float>(iz) * 0.052f,
                0.05f, 0.045f,
                0.885f, HorseEarTop);
        }

        builder.SetColor(materials.mane);

        // A stepped mane reinforces the rising rear edge of the neck.
        AddHorseSlab(builder, 
            0.105f, 0.0f,
            0.06f, 0.17f,
            0.63f, 0.77f);

        AddHorseSlab(builder, 
            0.18f, 0.0f,
            0.06f, 0.16f,
            0.73f, 0.86f);

        AddHorseSlab(builder, 
            0.26f, 0.0f,
            0.06f, 0.15f,
            0.81f, 0.90f);

        if (detailed)
        {
            builder.SetColor(materials.leather);

            // Bridle: a browband across the forehead and a noseband round
            // the muzzle, both standing proud of the head so they read as
            // straps rather than as stripes painted on it.
            AddHorseSlab(builder, 
                0.395f, 0.0f,
                0.045f, 0.185f,
                0.845f, 0.885f);

            AddHorseSlab(builder, 
                0.485f, 0.0f,
                0.05f, 0.15f,
                0.765f, 0.805f);

            // Cheek strap joining the two, on both sides of the head.
            for (int iz = -1; iz <= 1; iz += 2)
            {
                AddHorseSlab(builder, 
                    0.44f,
                    static_cast<float>(iz) * 0.078f,
                    0.13f, 0.032f,
                    0.79f, 0.86f);
            }

            // Reins running back from the noseband to the saddle. One slab
            // per side, which is all that survives at this scale.
            for (int iz = -1; iz <= 1; iz += 2)
            {
                AddHorseSlab(builder, 
                    0.25f,
                    static_cast<float>(iz) * 0.088f,
                    0.42f, 0.028f,
                    0.735f, 0.765f);
            }

            // Brass boss on the browband. Small, but it is what stops the
            // bridle reading as bare leather.
            builder.SetColor(materials.gold);

            AddHorseSlab(builder, 
                0.40f, 0.0f,
                0.05f, 0.05f,
                0.85f, 0.89f);
        }
    }

    float GetHorseTopY()
    {
        return HorseEarTop;
    }

    glm::vec3 GetHorseNeckPivot()
    {
        // The base of the first neck block, where it leaves the chest.
        return HorseVec(0.20f, 0.62f, 0.0f);
    }

    glm::vec3 GetHorseTailPivot()
    {
        return HorseVec(-0.30f, 0.66f, 0.0f);
    }

    glm::vec3 GetHorseLegPivot(int ix, int iz)
    {
        // Top of the upper leg, where it broadens into the body. Through
        // HorseVec like the geometry, so a pivot can never end up on a
        // different axis from the part it belongs to.
        return HorseVec(
            static_cast<float>(ix) * HorseLegAlong,
            HorseBellyY + 0.03f,
            static_cast<float>(iz) * HorseLegAcross);
    }

    glm::vec3 GetHorseBodyPivot()
    {
        // Mid-barrel. The body only ever rocks a degree or two about this,
        // so it matters far less than the limb joints do - but putting it
        // anywhere near the ground would swing the whole animal instead.
        return glm::vec3(0.0f, HorseBellyY + 0.1f, 0.0f);
    }

    //---------------------------------------------------------
    // Piece models
    //---------------------------------------------------------

    namespace
    {
        /// Builds one rig part: sets the origin to the joint, runs whatever
        /// the caller wants to put in it, and bakes the result.
        ///
        /// The origin is the whole trick. Every body builder in this file is
        /// written in ground-relative coordinates, and a part needs its joint
        /// at the mesh origin instead - so rather than a second set of
        /// builders, the origin is moved and the ordinary ones are used.
        template <typename BuildFn>
        void BuildPart(
            PieceRigModel& model,
            PieceJoint joint,
            const glm::vec3& pivot,
            BuildFn build)
        {
            MeshBuilder builder;

            builder.SetOrigin(pivot);

            build(builder);

            if (builder.IsEmpty())
                return;

            PieceRigPartModel& part = model.parts[static_cast<int>(joint)];

            part.mesh = builder.Build();
            part.pivot = pivot;
        }

        /// The parts every standing humanoid shares: two legs, a body and
        /// two arms, with whatever the piece carries baked into the arm that
        /// holds it.
        ///
        /// addBody is what stacks the torso, head and headwear, and addProp
        /// puts the sword, book or staff into the +armSpread hand - baking
        /// it into that arm's mesh is what guarantees the two can never come
        /// apart, whatever the animation does to the shoulder.
        template <typename BodyFn, typename PropFn>
        void BuildHumanoidRig(
            PieceRigModel& model,
            const FigureSpec& spec,
            float forwardReach,
            BodyFn addBody,
            PropFn addProp)
        {
            const float hipY = GetHipPivotY(spec);
            const float shoulderY = GetShoulderPivotY(spec);

            const glm::vec3 bodyPivot(0.0f, hipY, 0.0f);

            BuildPart(model, PieceJoint::Body, bodyPivot,
                [&](MeshBuilder& b) { addBody(b); });

            for (int side = -1; side <= 1; side += 2)
            {
                const PieceJoint legJoint = (side < 0)
                    ? PieceJoint::LeftLeg
                    : PieceJoint::RightLeg;

                BuildPart(
                    model, legJoint,
                    glm::vec3(
                        static_cast<float>(side) * spec.hipSpread,
                        hipY,
                        0.0f),
                    [&](MeshBuilder& b) { AddLeg(b, spec, side); });

                const PieceJoint armJoint = (side < 0)
                    ? PieceJoint::LeftArm
                    : PieceJoint::RightArm;

                BuildPart(
                    model, armJoint,
                    glm::vec3(
                        static_cast<float>(side) * spec.armSpread,
                        shoulderY,
                        0.0f),
                    [&](MeshBuilder& b)
                    {
                        AddArm(b, spec, side, forwardReach);

                        if (side > 0)
                            addProp(b);
                    });
            }
        }
    }

    bool HasRig(PieceType type)
    {
        // The four pieces with a rig. The rest still draw as one baked mesh,
        // which is all a piece that never moves needs.
        return type == PieceType::Pawn ||
            type == PieceType::Rook ||
            type == PieceType::Knight ||
            type == PieceType::Bishop;
    }

    PieceRigModel CreateRig(PieceType type)
    {
        PieceRigModel model;

        if (!HasRig(type))
            return model;

        model.baseWidth = 0.36f;
        model.baseDepth = 0.30f;

        switch (type)
        {
        case PieceType::Pawn:
        {
            FigureSpec spec;
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;

            const float handY = GetHandY(spec);

            BuildHumanoidRig(
                model, spec, 0.0f,
                [&](MeshBuilder& b)
                {
                    AddTorso(b, spec);
                    const float headTop = AddHead(b, spec);
                    AddSoldierHelmet(b, spec, headTop);
                },
                [&](MeshBuilder& b) { AddSword(b, spec, handY, 1.0f); });

            // Reported from the same builders the static model uses, so the
            // shadow and the spawn height do not shift when a piece is rigged.
            model.height = spec.headTop + 0.09f;
            break;
        }

        case PieceType::Rook:
        {
            FigureSpec spec;
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
            spec.shoulderWidth = 0.43f;
            spec.torsoWidth = 0.30f;

            const float handY = GetHandY(spec);

            float helmetTop = 0.0f;

            BuildHumanoidRig(
                model, spec, 0.0f,
                [&](MeshBuilder& b)
                {
                    AddTorso(b, spec);
                    const float headTop = AddHead(b, spec);
                    helmetTop = AddCastleHelmet(b, spec, headTop);
                },
                [&](MeshBuilder& b) { AddShield(b, spec, handY); });

            model.height = helmetTop;
            break;
        }

        case PieceType::Bishop:
        {
            FigureSpec spec;
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
            spec.shoulderWidth = 0.38f;
            spec.hipTop = 0.54f;
            spec.torsoTop = 0.74f;
            spec.shoulderBottom = 0.68f;
            spec.shoulderTop = 0.77f;
            spec.headTop = 0.91f;

            const float handY = GetHandY(spec);

            float mitreTop = 0.0f;

            BuildHumanoidRig(
                model, spec, 0.04f,
                [&](MeshBuilder& b)
                {
                    AddTorso(b, spec);
                    const float headTop = AddHead(b, spec);
                    mitreTop = AddMitre(b, spec, headTop);
                },
                [&](MeshBuilder& b) { AddBook(b, spec, handY); });

            // The robe stands in for the legs. It is a single skirt, so it
            // cannot stride - it hangs off the hips and sways, which is the
            // honest reading of a robed figure walking and the only one that
            // does not redesign the piece.
            BuildPart(
                model, PieceJoint::Robe,
                glm::vec3(0.0f, spec.hipTop, 0.0f),
                [&](MeshBuilder& b)
                {
                    AddRobe(b, spec, 0.42f, 0.28f, spec.hipTop);
                });

            model.height = mitreTop;
            model.baseWidth = 0.42f;
            model.baseDepth = 0.42f;
            break;
        }

        case PieceType::Knight:
        {
            const PieceMaterials& materials = PieceMaterialSets::Detailed;

            BuildPart(
                model, PieceJoint::Body, GetHorseBodyPivot(),
                [&](MeshBuilder& b) { AddHorseBody(b, materials); });

            BuildPart(
                model, PieceJoint::Head, GetHorseNeckPivot(),
                [&](MeshBuilder& b) { AddHorseNeckHead(b, materials); });

            BuildPart(
                model, PieceJoint::Tail, GetHorseTailPivot(),
                [&](MeshBuilder& b)
                {
                    b.SetColor(materials.coat);
                    AddHorseTail(b, materials);
                });

            // Front pair on +X, rear pair on -X. The humanoid's two leg
            // joints carry the front pair, so one animator covers both body
            // plans without a second set of names.
            const PieceJoint legJoints[2][2] =
            {
                { PieceJoint::RearLeftLeg, PieceJoint::RearRightLeg },
                { PieceJoint::LeftLeg, PieceJoint::RightLeg }
            };

            for (int ix = -1; ix <= 1; ix += 2)
            {
                for (int iz = -1; iz <= 1; iz += 2)
                {
                    const PieceJoint joint =
                        legJoints[(ix > 0) ? 1 : 0][(iz > 0) ? 1 : 0];

                    BuildPart(
                        model, joint, GetHorseLegPivot(ix, iz),
                        [&](MeshBuilder& b)
                        {
                            AddHorseLeg(b, materials, ix, iz);
                        });
                }
            }

            model.height = GetHorseTopY();
            model.baseWidth = 0.62f;
            model.baseDepth = 0.34f;
            break;
        }

        default:
            break;
        }

        model.valid = true;

        return model;
    }

    bool UsesDetailedMaterials(PieceType type)
    {
        // All seven current models now carry authored RGB vertex colours.
        // Leave their object colour white so team tinting cannot wash out
        // the shared material palette.
        return type == PieceType::Pawn ||
            type == PieceType::Rook ||
            type == PieceType::Knight ||
            type == PieceType::Bishop ||
            type == PieceType::Queen ||
            type == PieceType::King ||
            type == PieceType::MountedPawn;
    }

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
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;

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
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
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
            AddHorse(builder, PieceMaterialSets::Detailed);

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
            //
            // Reuse the Knight's detailed horse so its coat, tack, shading,
            // and gold fittings stay consistent across both mounted models.
            const float saddleY =
                AddHorse(builder, PieceMaterialSets::Detailed);

            FigureSpec spec;
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;

            // The rider faces the way the horse does. Now that the horse is
            // normalised to +Z, that is the ordinary camera facing rather
            // than the along-X one this used to need.
            spec.facing = Facing::Camera;

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

                builder.SetColor(spec.materials.armorLight);

                AddFigurePart(
                    builder, spec, across, 0.05f,
                    lift + 0.01f, spec.hipTop + 0.02f,
                    0.115f, 0.20f);

                builder.SetColor(spec.materials.clothBlue);

                AddFigurePart(
                    builder, spec, across, 0.125f,
                    lift - 0.20f, lift + 0.03f,
                    0.105f, 0.115f);

                builder.SetColor(spec.materials.boots);

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
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
            spec.shoulderWidth = 0.38f;
            spec.hipTop = 0.54f;
            spec.torsoTop = 0.74f;
            spec.shoulderBottom = 0.68f;
            spec.shoulderTop = 0.77f;
            spec.headTop = 0.91f;

            AddRobe(builder, spec, 0.42f, 0.28f, spec.hipTop);
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
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
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

            AddRobe(builder, spec, 0.44f, 0.28f, spec.hipTop);
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
            spec.materials = PieceMaterialSets::Detailed;
            spec.detailedFace = true;
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

const PieceMeshFactory::PieceRigModel& PieceMeshLibrary::GetRigModel(
    PieceType type)
{
    const int index = static_cast<int>(type);

    // A separate "built" flag rather than testing a mesh, because a type
    // with no rig legitimately has no meshes at all and would otherwise be
    // rebuilt from scratch on every single request.
    if (!rigModelBuilt[index])
    {
        rigModels[index] = PieceMeshFactory::CreateRig(type);
        rigModelBuilt[index] = true;
    }

    return rigModels[index];
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

    // Animated types swap the baked mesh for their rig. The shadow and the
    // dimensions above are deliberately taken from the baked model either
    // way, so rigging a type cannot change its footprint or its collision.
    if (PieceMeshFactory::HasRig(type))
    {
        const PieceMeshFactory::PieceRigModel& rigModel = GetRigModel(type);

        if (rigModel.valid)
        {
            auto rig = std::make_unique<PieceRig>();

            rig->Build(rigModel);

            piece->SetRigScale(GameConfig::PieceScale);

            piece->AttachRig(
                std::move(rig),
                type == PieceType::Knight);
        }
    }

    return piece;
}

void PieceMeshLibrary::Clear()
{
    for (PieceModel& model : models)
        model = PieceModel();

    for (PieceMeshFactory::PieceRigModel& model : rigModels)
        model = PieceMeshFactory::PieceRigModel();

    rigModelBuilt.fill(false);
}
