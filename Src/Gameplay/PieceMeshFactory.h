#pragma once

#include <array>
#include <memory>

#include <glm.hpp>

#include "ChessPiece.h"
#include "Mesh.h"
#include "MeshBuilder.h"

/// Compatibility palette retained from the older mesh implementation.
///
/// These are complete RGB colours, not brightness-only team multipliers.
/// PieceMaterialSets::Legacy below maps every named material back onto these
/// values so older callers can still request that compact material set.
namespace PiecePalette
{
    /// The dominant warm armour colour.
    inline const glm::vec3 Armor = glm::vec3(0.87f, 0.85f, 0.82f);

    // The dark end is kept well clear of black on purpose. The directional
    // light already pulls front faces down to about 0.63 of their colour, so
    // a material much below 0.4 loses all its internal shape and the robes
    // and capes turn into flat silhouettes.
    inline const glm::vec3 Metal = glm::vec3(0.72f, 0.74f, 0.78f);
    inline const glm::vec3 Skin = glm::vec3(0.83f, 0.68f, 0.56f);
    inline const glm::vec3 Cloth = glm::vec3(0.22f, 0.30f, 0.43f);
    inline const glm::vec3 Wood = glm::vec3(0.33f, 0.22f, 0.10f);
    inline const glm::vec3 Mane = glm::vec3(0.28f, 0.19f, 0.11f);
    inline const glm::vec3 Leather = glm::vec3(0.46f, 0.31f, 0.15f);
    inline const glm::vec3 Hair = glm::vec3(0.18f, 0.12f, 0.08f);
}

/// Every material a piece can be painted with, named by what it is rather
/// than by what colour it happens to be.
///
/// One of these is threaded through every part builder, so a builder never
/// names a colour of its own and the same skeleton can be painted two
/// different ways without duplicating a line of geometry. That is what lets
/// the detailed and compatibility palettes share the same builders.
///
/// The fields are deliberately finer-grained than the geometry needs today:
/// armorLight and armorShadow, leather and leatherDark, clothBlue and
/// clothLight all exist so a part can be split for contrast later without
/// another struct change. In the compatibility set the members of each such
/// pair hold the same value to preserve its simpler shading.
struct PieceMaterials
{
    // Flesh and hair.
    glm::vec3 skin;
    glm::vec3 hair;
    glm::vec3 eyes;

    // Plate, mail and trim.
    glm::vec3 armorLight;
    glm::vec3 armorShadow;
    glm::vec3 metalDark;
    glm::vec3 gold;

    /// Polished steel: blades, spearheads, shield plate.
    ///
    /// Separate from armorLight so blades and shield plate can keep a steel
    /// treatment independently of the armour colour.
    glm::vec3 blade;

    // Cloth worn under and around the plate.
    glm::vec3 clothBlue;
    glm::vec3 clothLight;

    // Belts, straps, tack and footwear.
    glm::vec3 leather;
    glm::vec3 leatherDark;
    glm::vec3 boots;

    // Hafts and shafts.
    glm::vec3 wood;

    // The horse: its coat, the darker muzzle, and mane and tail.
    glm::vec3 coat;
    glm::vec3 coatShadow;
    glm::vec3 mane;
};

/// The two palettes the pieces are painted from.
///
/// This is the single place any RGB value for a chess piece is written down.
namespace PieceMaterialSets
{
    /// The reference sheet's palette: warm skin, off-white plate over navy
    /// cloth, brown leather and muted gold.
    ///
    /// Nothing is pure white or pure black. The directional light already
    /// drops a front face to about 0.63 of its colour and lifts a top face
    /// to 0.91, so a material at 1.0 blows out on top and one near 0.0 goes
    /// to mud - both of which cost the shape its internal detail.
    inline const PieceMaterials Detailed =
    {
        glm::vec3(0.82f, 0.62f, 0.45f),   // skin
        glm::vec3(0.26f, 0.18f, 0.12f),   // hair
        glm::vec3(0.12f, 0.09f, 0.08f),   // eyes

        glm::vec3(0.86f, 0.86f, 0.83f),   // armorLight
        glm::vec3(0.55f, 0.56f, 0.58f),   // armorShadow
        glm::vec3(0.28f, 0.29f, 0.32f),   // metalDark
        glm::vec3(0.72f, 0.57f, 0.24f),   // gold
        glm::vec3(0.80f, 0.82f, 0.85f),   // blade

        glm::vec3(0.17f, 0.22f, 0.34f),   // clothBlue
        glm::vec3(0.38f, 0.44f, 0.55f),   // clothLight

        glm::vec3(0.45f, 0.31f, 0.19f),   // leather
        glm::vec3(0.28f, 0.19f, 0.12f),   // leatherDark
        glm::vec3(0.30f, 0.20f, 0.13f),   // boots

        glm::vec3(0.42f, 0.30f, 0.18f),   // wood

        glm::vec3(0.80f, 0.79f, 0.75f),   // coat
        glm::vec3(0.62f, 0.60f, 0.57f),   // coatShadow
        glm::vec3(0.28f, 0.24f, 0.20f)    // mane
    };

    /// The older compact RGB palette, one colour per named material group.
    ///
    /// Every pair that Detailed splits for contrast collapses back to a
    /// single value here, preserving the older flat-shaded treatment without
    /// maintaining a second set of mesh-generation functions.
    inline const PieceMaterials Legacy =
    {
        PiecePalette::Skin,      // skin
        PiecePalette::Hair,      // hair
        PiecePalette::Hair,      // eyes

        PiecePalette::Armor,     // armorLight
        PiecePalette::Armor,     // armorShadow
        PiecePalette::Metal,     // metalDark
        PiecePalette::Metal,     // gold
        PiecePalette::Metal,     // blade

        PiecePalette::Cloth,     // clothBlue
        PiecePalette::Cloth,     // clothLight

        PiecePalette::Leather,   // leather
        PiecePalette::Leather,   // leatherDark
        PiecePalette::Leather,   // boots

        PiecePalette::Wood,      // wood

        PiecePalette::Armor,     // coat
        PiecePalette::Armor,     // coatShadow
        PiecePalette::Mane       // mane
    };
}

/// A finished piece model plus the measurements the game needs from it.
///
/// The height is reported by the builder as it stacks the parts, rather
/// than written down separately, so it can never drift out of step with the
/// geometry.
struct PieceModel
{
    std::shared_ptr<Mesh> mesh;

    /// Ground to the highest point of the model.
    float height = 0.0f;

    /// Footprint on the ground, used to size the shadow. Width and depth are
    /// separate because the horse is long from nose to tail and narrow
    /// across, and a square shadow would spill into the next lane.
    float baseWidth = 0.0f;

    float baseDepth = 0.0f;
};

/// Builds the human-like chess piece models.
///
/// Every figure is the same skeleton of boxes - feet, lower legs, upper legs,
/// torso, shoulders, upper arms, lower arms, hands, head - with headwear and
/// one held prop deciding which piece it is. Sharing that skeleton is what
/// makes the set read as one army rather than seven unrelated models.
///
/// The rules from the abstract set still hold: large simple boxes, flat
/// colours, sharp edges, no rounded geometry, and nothing that fails to
/// change the silhouette.
///
/// Models are authored with y = 0 at the ground and centred on x/z, so the
/// pivot lands under the figure's feet and pieces rest naturally on whatever
/// they are placed on.
namespace PieceMeshFactory
{
    //---------------------------------------------------------
    // Figure space
    //---------------------------------------------------------

    /// Which way a figure looks.
    ///
    /// Standing pieces face the camera so their chest and face are visible.
    /// A rider faces along the horse instead. Body parts are authored once in
    /// figure space - width across the shoulders, depth front to back - and
    /// mapped onto world axes by this, so one set of builders serves both.
    enum class Facing
    {
        Camera,
        AlongX
    };

    /// Proportions of one humanoid figure, all ground-relative.
    ///
    /// The defaults are the standard soldier used by the pawn and the rook.
    /// The taller and bulkier pieces override individual fields rather than
    /// redefining the whole body, which keeps the set proportional.
    struct FigureSpec
    {
        Facing facing = Facing::Camera;

        /// What this figure is painted with.
        ///
        /// It rides on the spec rather than being passed to every builder
        /// separately because the spec is already threaded through all of
        /// them, and because "what this figure is made of" belongs with
        /// "what shape this figure is".
        ///
        /// Defaulting to the compatibility set keeps helper callers safe;
        /// every current detailed humanoid selects Detailed explicitly.
        PieceMaterials materials = PieceMaterialSets::Legacy;

        /// Whether to cut a face - eyes and a fringe of hair - into the
        /// head, and to split the plate into lit and shadowed halves.
        ///
        /// Off by default so helper callers opt into the extra geometry. It
        /// is the one place detailed figures add boxes rather than only
        /// colour, and they add them for the reason the reference does: a
        /// bare skin-coloured block reads as a thumb, not a head.
        bool detailedFace = false;

        // Widths, measured across the figure.
        float shoulderWidth = 0.40f;
        float torsoWidth = 0.28f;
        float torsoDepth = 0.17f;
        float hipSpread = 0.075f;
        float armSpread = 0.185f;
        float headWidth = 0.19f;
        float headDepth = 0.17f;

        // Vertical landmarks, in stacking order.
        float feetTop = 0.075f;
        float kneeTop = 0.25f;
        float hipTop = 0.43f;
        float torsoTop = 0.64f;
        float shoulderBottom = 0.58f;
        float shoulderTop = 0.68f;
        float headTop = 0.82f;
    };

    /// Adds one box in figure space: offset across the body and forward from
    /// its centre, spanning a vertical range, with a width and a depth.
    void AddFigurePart(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float across,
        float forward,
        float bottomY,
        float topY,
        float width,
        float depth);

    //---------------------------------------------------------
    // Body parts
    //
    // Each returns the Y it finished at, so parts chain by passing the
    // result of one into the next.
    //---------------------------------------------------------

    /// Feet, lower legs and upper legs, for a figure that stands.
    float AddLegs(
        MeshBuilder& builder,
        const FigureSpec& spec);

    /// A flared skirt in place of legs, for the bishop and the queen. One
    /// frustum, so the robe has genuinely sloped sides rather than steps.
    float AddRobe(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomWidth,
        float topWidth,
        float topY);

    /// Torso, belt and shoulders.
    float AddTorso(
        MeshBuilder& builder,
        const FigureSpec& spec);

    /// Upper arms, lower arms and hands, hanging at the sides. The hands end
    /// up at the height returned, which is where a held prop belongs.
    float AddArms(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float forwardReach = 0.0f);

    /// The bare head. Headwear is added on top of the returned Y.
    float AddHead(
        MeshBuilder& builder,
        const FigureSpec& spec);

    //---------------------------------------------------------
    // Headwear
    //---------------------------------------------------------

    /// The pawn's plain helmet: a brim and a dome.
    float AddSoldierHelmet(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y);

    /// The rook's helmet, battlemented like a castle tower.
    float AddCastleHelmet(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y);

    /// The bishop's tall mitre: a band and a long taper.
    float AddMitre(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y);

    /// A banded crown with four corner points.
    ///
    /// The queen wears tall thin points with a taller fifth in the middle;
    /// the king wears short thick ones under a cross. Same helper, opposite
    /// proportions - that contrast is what stops the two crowns, and the
    /// rook's battlements, reading as the same two bumps on a head.
    float AddCrown(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float y,
        float pointHeight,
        bool centrePoint,
        bool withCross);

    /// Four chunky blocks at the corners of a square ring. Shared by the
    /// rook's battlements and the crowns, which are the same idea at
    /// different proportions.
    float AddCrenellations(
        MeshBuilder& builder,
        float ringWidth,
        float blockWidth,
        float y,
        float height);

    /// An upright with one arm across it, for the top of the king's crown.
    float AddCross(
        MeshBuilder& builder,
        float y,
        float height,
        float barWidth,
        float armWidth);

    //---------------------------------------------------------
    // Held props
    //
    // Each is placed relative to a hand, so moving an arm moves the prop.
    //---------------------------------------------------------

    /// A sword held point-down. The king's is longer and broader.
    void AddSword(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY,
        float scale);

    /// The rook's shield, carried on its far side and facing forward.
    void AddShield(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY);

    /// The bishop's book, held up against the chest.
    void AddBook(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY);

    /// The queen's scepter, standing taller than her crown.
    float AddStaff(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float handY,
        float topY);

    /// A cape hanging down the figure's back, wider at the hem.
    void AddCape(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY);

    /// Long hair falling behind the head, for the queen.
    void AddHair(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY,
        float topY);

    /// The king's beard, on the front of his jaw.
    void AddBeard(
        MeshBuilder& builder,
        const FigureSpec& spec,
        float bottomY,
        float topY);

    //---------------------------------------------------------
    // Horse
    //---------------------------------------------------------

    /// The knight's horse, facing along +X, saddled and ready to carry a
    /// rider. Returns the Y of the saddle's seat, which is where a mounted
    /// figure's legs straddle it.
    float AddHorse(
        MeshBuilder& builder,
        const PieceMaterials& materials);

    /// The horse in the four pieces the rig splits it into. AddHorse is
    /// these in order, so the static animal and the animated one cannot
    /// diverge.
    ///
    /// ix picks front (+1) from rear (-1) and iz picks the side, which is
    /// how the rig tells one leg from another.
    void AddHorseLeg(
        MeshBuilder& builder,
        const PieceMaterials& materials,
        int ix,
        int iz);

    void AddHorseBody(
        MeshBuilder& builder,
        const PieceMaterials& materials);

    void AddHorseNeckHead(
        MeshBuilder& builder,
        const PieceMaterials& materials);

    void AddHorseTail(
        MeshBuilder& builder,
        const PieceMaterials& materials);

    //---------------------------------------------------------
    // Horse joint landmarks
    //---------------------------------------------------------

    /// Where the neck meets the chest, and where the tail leaves the rump.
    glm::vec3 GetHorseNeckPivot();

    glm::vec3 GetHorseTailPivot();

    /// The shoulder or hip one of the four legs swings about.
    glm::vec3 GetHorseLegPivot(int ix, int iz);

    /// The point the body itself rocks about.
    glm::vec3 GetHorseBodyPivot();

    /// Highest point of the horse, so the knight can report its height
    /// without the caller having to know how the horse is put together.
    float GetHorseTopY();

    //---------------------------------------------------------
    // Piece models
    //---------------------------------------------------------

    //---------------------------------------------------------
    // Animated rigs
    //
    // The same models again, but split into the parts that have to move
    // independently instead of baked into one mesh. Both forms are built by
    // the very same part builders - the split versions only set the mesh
    // origin to the joint first - so a rig can never drift out of step with
    // the static model it came from.
    //---------------------------------------------------------

    /// The parts an animated piece can be split into.
    ///
    /// One list covers both body plans. A humanoid uses Root, Body, the two
    /// arms and the two legs, plus Robe if it wears one instead of legs. The
    /// horse uses Root, Body, Head and Tail, and all four legs - its front
    /// pair reusing the humanoid's LeftLeg and RightLeg rather than adding
    /// two more names that would mean the same thing.
    ///
    /// A model simply leaves the joints it has no use for empty.
    enum class PieceJoint
    {
        /// The whole figure. Carries the walk bob and the jump crouch, and
        /// nothing else - it is the one joint with no mesh of its own.
        Root,

        /// Everything above the hips: torso, shoulders, head, headwear.
        Body,

        /// The horse's neck and head. Unused by the humanoids.
        Head,

        Tail,

        /// A skirt in place of legs, for the robed figures.
        Robe,

        /// A figure carried by another model: the pawn sitting on the horse.
        ///
        /// It hangs off the horse's Body, so every bounce and rock the horse
        /// makes carries the rider with it and the two can never drift
        /// apart. Its own arms hang off this in turn.
        Rider,

        LeftArm,
        RightArm,

        LeftLeg,
        RightLeg,

        RearLeftLeg,
        RearRightLeg,

        Count
    };

    constexpr int PieceJointCount = static_cast<int>(PieceJoint::Count);

    /// One part of an animated model.
    struct PieceRigPartModel
    {
        std::shared_ptr<Mesh> mesh;

        /// Where this part's joint sits in the model's own coordinates.
        ///
        /// The mesh is authored with this point at its origin, so the node
        /// that carries it rotates about the joint and nothing else. The
        /// pivot is kept here as well because a child's position relative to
        /// its parent is the difference between the two pivots, and only the
        /// rig can work that out.
        glm::vec3 pivot = glm::vec3(0.0f);

        /// The joint this part hangs off.
        ///
        /// Carried per model rather than derived from the joint's identity,
        /// because the same joint does not always hang off the same thing: a
        /// standing figure's arms follow its torso, while a rider's follow
        /// the rider, which itself follows the horse. One static map cannot
        /// express both, and guessing from the joint name is how a rider
        /// ends up swinging from the horse's ribs.
        PieceJoint parent = PieceJoint::Root;
    };

    /// A finished animated model: one mesh per joint, plus the same
    /// measurements the baked model reports.
    struct PieceRigModel
    {
        std::array<PieceRigPartModel, PieceJointCount> parts;

        float height = 0.0f;
        float baseWidth = 0.0f;
        float baseDepth = 0.0f;

        /// False for the pieces that have no rig and still draw as one mesh.
        bool valid = false;
    };

    /// Whether this piece type has an animated rig.
    bool HasRig(PieceType type);

    /// Builds the split version of a model. Returns an invalid model for the
    /// types that do not have one.
    PieceRigModel CreateRig(PieceType type);

    //---------------------------------------------------------
    // Joint landmarks
    //
    // Where the animated joints sit on a figure. Derived from the same spec
    // the geometry is, so a pivot cannot end up somewhere the body is not.
    //---------------------------------------------------------

    /// Height of the hip joint the legs swing about.
    float GetHipPivotY(const FigureSpec& spec);

    /// Height of the shoulder joint the arms swing about.
    float GetShoulderPivotY(const FigureSpec& spec);

    /// Height the hands end up at, which is where a held prop is gripped.
    float GetHandY(const FigureSpec& spec);

    /// One leg or one arm, for building a rig part. AddLegs and AddArms are
    /// these in a loop, so the static and animated models stay identical.
    void AddLeg(
        MeshBuilder& builder,
        const FigureSpec& spec,
        int side);

    void AddArm(
        MeshBuilder& builder,
        const FigureSpec& spec,
        int side,
        float forwardReach = 0.0f);

    /// Whether a piece is painted from PieceMaterialSets::Detailed.
    ///
    /// The repainted pieces carry real colours in their vertices, so their
    /// object colour has to be left white or the team tint would multiply
    /// straight through the palette and undo it. ChessPiece asks this before
    /// deciding what to tint itself, which keeps the two ends of that rule
    /// from drifting apart.
    bool UsesDetailedMaterials(PieceType type);

    /// Builds the model for one piece type.
    ///
    /// To add a piece, extend PieceType, its count and name mapping, and add
    /// one case here. The mesh library, showcase loop and renderer all stay
    /// type-agnostic.
    PieceModel Create(PieceType type);
}

/// Owns one model per piece type and hands them out.
///
/// Models are built the first time they are asked for and then shared by
/// every piece of that type, so a hundred pawns still cost one vertex
/// buffer. The library must outlive the pieces it created and be destroyed
/// while the GL context still exists, which is why the game owns it.
class PieceMeshLibrary
{
public:

    PieceMeshLibrary();

    /// Returns the model for a type, building it on first use.
    const PieceModel& GetModel(PieceType type);

    /// The split, animatable version of a model, built on first use.
    ///
    /// Cached beside the baked one for the same reason: the meshes are
    /// shared by every piece of that type, so a dozen animated pawns still
    /// cost one set of vertex buffers between them.
    const PieceMeshFactory::PieceRigModel& GetRigModel(PieceType type);

    /// Factory: a ready-to-render piece sharing this library's model.
    std::shared_ptr<ChessPiece> CreatePiece(
        PieceType type,
        PieceTeam team = PieceTeam::White);

    /// Releases every model. Call before the GL context goes away.
    void Clear();

private:

    std::array<PieceModel, PieceTypeCount> models;

    std::array<PieceMeshFactory::PieceRigModel, PieceTypeCount> rigModels;

    std::array<bool, PieceTypeCount> rigModelBuilt{};
};
