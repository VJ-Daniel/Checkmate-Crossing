#pragma once

#include <array>
#include <memory>

#include <glm.hpp>

#include "Mesh.h"
#include "MeshBuilder.h"
#include "ObstacleMeshFactory.h"

/// Flat materials the checkpoint gate is painted with.
///
/// Deliberately the props' own stone and timber rather than a fresh set of
/// values. The gate's wall segments have to run straight into an
/// ObstacleType::Wall without a seam showing, and two greys that are nearly
/// the same read far worse than one grey used twice.
namespace GatePalette
{
    inline const glm::vec3 Stone = ObstaclePalette::Stone;
    inline const glm::vec3 DarkStone = ObstaclePalette::DarkStone;
    inline const glm::vec3 Wood = ObstaclePalette::Wood;
    inline const glm::vec3 DarkWood = ObstaclePalette::DarkWood;
    inline const glm::vec3 Iron = ObstaclePalette::Iron;

    /// Heraldry - the one thing the gate adds to the palette.
    ///
    /// Kept duller and darker than ObstaclePalette::Leaf on purpose: a
    /// banner painted in foliage green reads as a bush stuck to a wall.
    inline const glm::vec3 Banner = glm::vec3(0.22f, 0.42f, 0.24f);
    inline const glm::vec3 BannerFold = glm::vec3(0.15f, 0.30f, 0.17f);
    inline const glm::vec3 Emblem = glm::vec3(0.90f, 0.89f, 0.84f);

    /// Darkens every second course of masonry.
    ///
    /// The same trick the lanes use to stay readable without a texture.
    /// Without it a pillar is one grey box: the stacked-block look the gate
    /// is meant to have comes entirely from this alternation plus the small
    /// step between courses, since there is nothing else to draw it with.
    inline glm::vec3 Course(const glm::vec3& color, int index)
    {
        return (index % 2 == 0) ? color : color * 0.92f;
    }
}

/// Every measurement of the gate, in one place.
///
/// The gate is the first model in the project made of parts that have to
/// agree with one another at run time rather than inside a single mesh: the
/// doors meet in the middle of the gateway, the hinges sit on the faces of
/// the pillars, and the wall run starts where the pillars stop. Naming those
/// numbers once and deriving the rest is what makes "both leaves are aligned
/// when shut" true by construction instead of true until someone re-tunes a
/// width.
namespace GateMetrics
{
    //---------------------------------------------------------
    // Gateway
    //---------------------------------------------------------

    /// Clear span between the pillars. Wide enough that the pawn walks
    /// through it with room either side rather than threading a slot.
    constexpr float OpeningWidth = 1.60f;

    /// One leaf covers exactly half the opening, so the two meet on the
    /// centre line with no gap and no overlap.
    constexpr float DoorWidth = OpeningWidth * 0.5f;

    constexpr float DoorThickness = 0.16f;

    //---------------------------------------------------------
    // Pillars
    //---------------------------------------------------------

    /// Chunky on purpose. The pillar has to stay obviously stone with a
    /// banner hanging on it, so it is kept wide enough to show a clear
    /// margin of masonry either side of the cloth.
    constexpr float PillarWidth = 0.74f;
    constexpr float PillarDepth = 0.50f;

    /// Top of the shaft. The cap is a separate piece that sits on this.
    constexpr float PillarHeight = 1.52f;

    constexpr float PillarBaseWidth = 0.86f;
    constexpr float PillarBaseDepth = 0.60f;
    constexpr float PillarBaseHeight = 0.14f;

    constexpr int PillarCourses = 4;

    /// How far a recessed course steps sideways. Also how far it is inset on
    /// each face, so the step always lands away from the gateway and the
    /// shaft never grows into the doors' swing.
    constexpr float CourseStep = 0.02f;

    /// Pillars stand hard against the opening, so their centres are half an
    /// opening plus half a pillar out from the middle of the gate. Every
    /// other X in the structure is measured from this.
    constexpr float PillarCenterX = DoorWidth + PillarWidth * 0.5f;

    /// Outer face of the pillars, where the wall run begins.
    constexpr float CoreHalfWidth = PillarCenterX + PillarWidth * 0.5f;

    //---------------------------------------------------------
    // Stone cap
    //---------------------------------------------------------

    constexpr float CapBandWidth = 0.82f;
    constexpr float CapBandDepth = 0.58f;
    constexpr float CapBandHeight = 0.12f;

    constexpr float CapTopWidth = 0.90f;
    constexpr float CapTopDepth = 0.66f;
    constexpr float CapHeight = 0.26f;

    //---------------------------------------------------------
    // Door leaf
    //---------------------------------------------------------

    constexpr float DoorLatticeHeight = 1.24f;

    /// The plank back the lattice is nailed to.
    ///
    /// Without it the leaves are a portcullis: the grid alone lets the lane
    /// behind show straight through, and a checkpoint gate has to look like
    /// something that shuts.
    constexpr float DoorBackingThickness = 0.05f;

    /// Iron strap across the top of the timber, carrying the spikes.
    constexpr float DoorBandHeight = 0.10f;

    /// The spikes reach exactly the top of the pillar shafts, so their tips
    /// line up with the underside of the caps the way the reference does.
    /// Derived rather than measured, so re-tuning the pillars moves them.
    constexpr float DoorSpikeHeight =
        PillarHeight - DoorLatticeHeight - DoorBandHeight;

    constexpr float DoorHeight = PillarHeight;

    constexpr float DoorStileWidth = 0.11f;

    /// A sill across the foot of the leaf, in the frame's own timber.
    ///
    /// There for the reason a real door has one. The leaf is thick enough
    /// that its front face reaches further down the screen than the panel
    /// behind it does, so without a sill the bottom edge comes out ragged,
    /// with ground showing between the uprights.
    constexpr float DoorSillHeight = 0.12f;

    constexpr float DoorBarWidth = 0.075f;
    constexpr int DoorBarCount = 3;

    constexpr float DoorRailHeight = 0.10f;
    constexpr int DoorRailCount = 4;

    constexpr float DoorStudSize = 0.05f;

    constexpr float DoorSpikeBase = 0.09f;
    constexpr int DoorSpikeCount = 3;

    /// Widest a leaf may swing: at a right angle it lies flat along its
    /// pillar. Past that it starts sweeping back across the wall run.
    constexpr float MaxDoorAngle = 90.0f;

    //---------------------------------------------------------
    // Wall run
    //
    // These are ObstacleType::Wall's own numbers. A segment placed beside a
    // wall prop has to show the same height and the same overhang or the
    // gate reads as a separate structure dropped next to the wall instead of
    // cut into it.
    //---------------------------------------------------------

    constexpr float WallBodyWidth = 0.94f;
    constexpr float WallBodyDepth = 0.28f;
    constexpr float WallBodyHeight = 0.64f;

    constexpr float WallCapWidth = 1.02f;
    constexpr float WallCapDepth = 0.34f;

    constexpr float WallHeight = 0.78f;

    constexpr int WallCourses = 2;

    /// Segments are placed slightly closer than their caps are wide, so
    /// neighbours overlap by a couple of centimetres and the run never
    /// shows a hairline of background between two blocks.
    constexpr float WallSpacing = 1.00f;

    //---------------------------------------------------------
    // Banner
    //---------------------------------------------------------

    /// Narrower than the pillar it hangs on by a good margin either side.
    /// A banner that covers its own pillar turns the masonry green.
    constexpr float BannerWidth = 0.30f;
    constexpr float BannerThickness = 0.045f;
    constexpr float BannerRodWidth = 0.40f;
    constexpr float BannerHeight = 0.92f;

    /// How high off the ground the swallowtail hangs. Chosen so the rod
    /// clears the top of the wall run and the banner still stops short of
    /// the cap, leaving bare stone at both ends of the shaft.
    constexpr float BannerHangY = 0.50f;
}

/// The reusable pieces one checkpoint gate is assembled from.
///
/// Each is its own mesh with its own transform, which is what lets the two
/// leaves turn independently while the masonry stays put. Splitting the
/// masonry up as well costs nothing - the pieces share one library - and
/// makes the set modular: a wall run is the same segment repeated, and the
/// cap fits a pillar or a wall equally.
enum class GatePart
{
    LeftPillar,
    RightPillar,
    LeftDoor,
    RightDoor,
    WallSegment,
    StoneCap,
    Banner
};

constexpr int GatePartCount = 7;

/// A finished part model plus the measurements the gate needs from it.
struct GatePartModel
{
    std::shared_ptr<Mesh> mesh;

    /// Base of the part to its highest point.
    float height = 0.0f;

    float width = 0.0f;

    float depth = 0.0f;
};

/// Builds the checkpoint gate's parts.
///
/// Same rules as the chess pieces and the props: large simple boxes, flat
/// colours, sharp edges, and nothing that does not change the silhouette.
///
/// Two authoring conventions matter here, because the parts have to line up
/// with one another once they are placed:
///
///   - Masonry is authored with y = 0 at the ground and centred on x and z,
///     exactly like every other model, so a piece is placed by naming the
///     patch of ground it stands on. The cap is the one exception: it is
///     authored from its own base so it can be dropped on top of anything.
///
///   - A door leaf is authored with its origin on the hinge - at the edge
///     that meets the pillar and at the leaf's back face, not its centre
///     line. That single choice is what makes the hinge correct: the leaf
///     occupies only the quarter-plane in front of and away from its pivot,
///     so swinging it through a right angle sweeps it forward out of the
///     gateway and never drives a corner into the masonry it hangs on.
namespace GateMeshFactory
{
    //---------------------------------------------------------
    // Shared parts
    //---------------------------------------------------------

    /// A stack of masonry courses, alternately stepped and shaded.
    ///
    /// Takes its base colour from the builder's current colour and leaves it
    /// unchanged, so callers name a material once as they do everywhere else.
    ///
    /// sideStep pushes every second course along X and insets it by the same
    /// amount on each face. Passing the sign that points away from the
    /// gateway keeps the stepped faces on the outside, where they cannot
    /// grow into the space a leaf swings through.
    ///
    /// Returns the Y it finished at, so parts chain together.
    float AddCourses(
        MeshBuilder& builder,
        float x,
        float z,
        float width,
        float depth,
        float bottomY,
        float topY,
        int courses,
        float sideStep);

    /// The stepped capstone, authored from its own base at y = 0.
    float AddStoneCap(
        MeshBuilder& builder,
        float bottomY);

    /// One leaf of the gate: frame, lattice, ironwork and spikes.
    ///
    /// side is +1 for a leaf that extends along +X from its hinge and -1 for
    /// one that extends along -X. Everything inside is written once in
    /// "door space" - distance out from the hinge - so the two leaves are
    /// the same geometry read in opposite directions and cannot drift out of
    /// step with each other.
    void AddDoorLeaf(
        MeshBuilder& builder,
        float side);

    /// The rook device carried on a banner: flat plates standing just proud
    /// of the cloth, not a model of a rook.
    void AddRookEmblem(
        MeshBuilder& builder,
        float x,
        float bottomY);

    //---------------------------------------------------------
    // Part models
    //---------------------------------------------------------

    /// To add a part, extend the enum and its count and add one case here.
    /// CheckpointGate is the only thing that knows how the parts fit
    /// together; the library and the renderer stay part-agnostic.
    GatePartModel Create(GatePart part);
}

/// Owns one model per gate part and hands them out.
///
/// Same contract as PieceMeshLibrary and ObstacleMeshLibrary: models are
/// built on first use and shared by every gate, so a checkpoint on every
/// section of the level still costs one vertex buffer per part. The library
/// must be destroyed while the GL context is still alive.
class GateMeshLibrary
{
public:

    GateMeshLibrary();

    const GatePartModel& GetModel(GatePart part);

    void Clear();

private:

    std::array<GatePartModel, GatePartCount> models;
};
