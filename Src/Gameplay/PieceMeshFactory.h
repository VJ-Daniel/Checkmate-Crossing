#pragma once

#include <array>
#include <memory>

#include <glm.hpp>

#include "ChessPiece.h"
#include "Mesh.h"
#include "MeshBuilder.h"

/// Tonal materials the figures are built from.
///
/// These are deliberately hueless. Every value is a brightness multiplier
/// applied on top of the piece's team colour, so a white piece stays white
/// and a black piece stays black - which is what a chess set wants - while
/// armour, cloth, leather and steel still separate from one another.
///
/// The reference image's blue and gold heraldry is intentionally ignored;
/// only the tonal relationships are borrowed from it.
namespace PiecePalette
{
    /// The dominant material, left at full team colour.
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
        MeshBuilder& builder);

    /// Highest point of the horse, so the knight can report its height
    /// without the caller having to know how the horse is put together.
    float GetHorseTopY();

    //---------------------------------------------------------
    // Piece models
    //---------------------------------------------------------

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

    /// Factory: a ready-to-render piece sharing this library's model.
    std::shared_ptr<ChessPiece> CreatePiece(
        PieceType type,
        PieceTeam team = PieceTeam::White);

    /// Releases every model. Call before the GL context goes away.
    void Clear();

private:

    std::array<PieceModel, PieceTypeCount> models;
};
