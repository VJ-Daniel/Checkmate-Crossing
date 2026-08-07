#pragma once

#include <glm.hpp>

/*
    Central tuning values for the battlefield and the camera that frames it.

    Everything that decides "how the game looks" lives here so the view can
    be re-tuned without touching rendering or gameplay code.
*/
namespace GameConfig
{
    //---------------------------------------------------------
    // World metrics
    //---------------------------------------------------------

    /// One lane (one row of the battlefield) is one world unit deep.
    /// Free movement and level row conversion share this world-space unit.
    constexpr float TileSize = 1.0f;

    /// Walkable width of the battlefield, in tiles.
    /// Nine keeps the chessboard feel while leaving a centre column.
    constexpr int BoardWidthInTiles = 9;

    /// Lanes are drawn wider than the walkable board so the ground always
    /// reaches both screen edges, exactly like the reference game.
    constexpr float LaneDrawWidth = 24.0f;

    /// How far the board extends below its surface.
    ///
    /// Lanes are solid blocks rather than flat planes, so the battlefield
    /// has a real edge all the way round instead of a paper-thin one.
    /// Nothing inside the board shows it: the floor is flat end to end, so
    /// the only vertical faces left are around the perimeter, well outside
    /// the view.
    constexpr float BoardBaseThickness = 0.7f;

    /// Height of the walking surface - for every lane in the level.
    ///
    /// One value, deliberately. The battlefield is a single flat floor and a
    /// lane type is paint on it, not terracing: what makes a row dangerous
    /// is the obstacles and hazards crossing it, not a step down into it.
    ///
    /// Zero, so a model authored with y = 0 at its base stands at world
    /// y = 0 with no correction anywhere.
    ///
    /// Depth in the orthographic view therefore comes entirely from what
    /// stands on the floor - the props, the scenery border and the
    /// checkpoint gate - rather than from the floor itself.
    constexpr float GroundSurface = 0.0f;

    //---------------------------------------------------------
    // Finished outer boundary
    //---------------------------------------------------------

    /// Clear ground between the playable rectangle and the first decorative
    /// boundary block. The pawn is constrained by the playable rectangle;
    /// the blocks begin just outside it and never cover the road.
    constexpr float BoundaryInnerClearance = 0.25f;

    /// Depth of the dense rock/greenery band outside every map edge. This is
    /// deliberately wider than a single row: the tilted orthographic camera
    /// sees several ground units beyond its target, and the finished band is
    /// the visual envelope used by the camera clamp.
    constexpr float BoundaryThickness = 4.5f;

    constexpr int BoundaryLayerCount = 5;

    /// Nominal distance between blocks along an edge. Their minimum length
    /// is larger than this, so the inner silhouette has no direct sight gap.
    constexpr float BoundaryBlockSpacing = 1.0f;

    constexpr float BoundaryMinHeight = 0.55f;
    constexpr float BoundaryMaxHeight = 2.15f;

    constexpr float BoundaryMinAlongSize = 1.08f;
    constexpr float BoundaryMaxAlongSize = 1.34f;

    /// Always wider than the 0.9-unit layer spacing, guaranteeing overlap
    /// between neighbouring depth rows even before rotation is considered.
    constexpr float BoundaryMinAcrossSize = 0.94f;
    constexpr float BoundaryMaxAcrossSize = 1.12f;

    constexpr float BoundaryAlongJitter = 0.035f;
    constexpr float BoundaryMaxRotationDegrees = 8.0f;

    /// Stable coordinate-hash seed. Boundary variation is identical on
    /// every launch and independent per edge, layer and block.
    constexpr unsigned int BoundaryVariationSeed = 20260729u;

    //---------------------------------------------------------
    // Camera
    //
    // The reference game uses an orthographic camera tilted down toward the
    // player, pointing along -Z and never rotated by the player.
    //
    // The GDD camera hint gives the same angle:
    //
    //     camera position = player position + (0, 12, 10)
    //
    //     pitch    = atan(12 / 10)          = 50.2 degrees below the horizon
    //     distance = sqrt(12^2 + 10^2)      = 15.62 world units
    //---------------------------------------------------------

    constexpr float CameraPitchDegrees = 35.0f;

    /// Swinging the camera off the lane axis is what turns the view from a
    /// striped board into a solid 3D world: it tilts the lanes across the
    /// screen and exposes a second vertical face on every block.
    ///
    /// A lane running along X lands on screen at an angle of
    ///     atan( tan(yaw) * sin(pitch) )
    /// so 24 degrees of yaw tilts the lanes by about 19 degrees.
    constexpr float CameraYawDegrees = 18.0f;

    constexpr float CameraDistance = 15.62f;

    /// Vertical slice of the world the orthographic camera shows: the "zoom".
    /// Measured from the reference screenshot, which shows about 7.3 lanes
    /// from top to bottom:  7.3 lanes * sin(50.2 degrees) = 5.6 world units.
    constexpr float CameraOrthographicHeight = 6.3f;

    /// The camera aims this far past the pawn, which pushes the pawn below
    /// the middle of the screen and leaves the upper two thirds for the road
    /// ahead. The pawn ends up around 65% of the way down the view.
    constexpr float CameraLookAhead = 1.2f;

    /// World-space lock region around the camera target. Movement inside it
    /// moves the pawn on screen without nudging the camera; crossing an edge
    /// advances only far enough to put the pawn back on that edge.
    constexpr float CameraDeadZoneWidth = 2.4f;
    constexpr float CameraDeadZoneDepth = 1.5f;

    /// Keeps the calculated ground footprint a little inside the nominal
    /// outer boundary, absorbing floating-point and rasterization slop.
    constexpr float CameraBoundsInset = 0.12f;

    /// Used only when the camera is switched to perspective mode. A narrow
    /// field of view keeps the framing close to the orthographic look.
    constexpr float CameraFieldOfView = 25.0f;

    constexpr float CameraNearPlane = 0.1f;

    constexpr float CameraFarPlane = 200.0f;

    //---------------------------------------------------------
    // Pawn placeholder
    //---------------------------------------------------------

    constexpr float PawnWidth = 0.6f;

    constexpr float PawnHeight = 0.9f;

    /// GDD: "White or light-coloured cube for the pawn placeholder."
    inline const glm::vec4 PawnColor = glm::vec4(0.93f, 0.92f, 0.88f, 1.0f);

    //---------------------------------------------------------
    // Chess piece placeholders
    //
    // Flat team colours only. The models are built from plain cubes, so
    // the shading is what separates their faces; a mid-grey for the black
    // team keeps its shadowed faces from collapsing into pure black.
    //---------------------------------------------------------

    inline const glm::vec4 WhitePieceColor =
        glm::vec4(0.87f, 0.86f, 0.83f, 1.0f);

    inline const glm::vec4 BlackPieceColor =
        glm::vec4(0.30f, 0.30f, 0.33f, 1.0f);

    /// Shadow width relative to a piece's base. Pieces have wider feet than
    /// the pawn placeholder, so they need less padding.
    constexpr float PieceShadowScale = 1.25f;

    /// Uniform scale applied to every piece model.
    ///
    /// The models are authored at their natural proportions and then scaled
    /// as a whole, which keeps one shared mesh per type. Pieces have to hold
    /// their own beside the chunky scenery, so they sit a little over half a
    /// tile wide - matching the character in the reference world.
    constexpr float PieceScale = 1.2f;

    //---------------------------------------------------------
    // Lighting
    //
    // One directional light plus ambient. The camera yaw means three faces
    // of every block are visible at once - the top, the front (+Z) and the
    // right (+X) - so the light is aimed from above and to the left to give
    // those three faces clearly different brightness:
    //
    //     top    0.91
    //     front  0.63
    //     right  0.34   (turned away from the light: ambient only)
    //
    // Getting this wrong is what makes blocks look flat. A light pointing
    // straight down the lanes would leave the front and side faces almost
    // equally bright and the shapes would read as cardboard cut-outs.
    //---------------------------------------------------------

    inline const glm::vec3 LightDirection =
        glm::normalize(glm::vec3(0.25f, -0.88f, -0.45f));

    constexpr float AmbientStrength = 0.34f;

    //---------------------------------------------------------
    // Fake shadow
    //
    // A dark translucent quad laid on the ground under the pawn. Real
    // shadow mapping is a much larger change; this costs one draw call and
    // is what anchors the pawn to the ground instead of letting it float.
    //---------------------------------------------------------

    inline const glm::vec4 ShadowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.32f);

    /// How much wider the shadow is than the pawn itself.
    constexpr float ShadowScale = 1.45f;

    /// Lifted just clear of the ground so the two surfaces do not z-fight.
    constexpr float ShadowGroundOffset = 0.02f;

    //---------------------------------------------------------
    // Chess-piece abilities (GDD section 3)
    //
    // NOTE(Ayub): the GDD names each ability's effect but not its exact
    // numbers. Values below are placeholder tuning -- adjust freely.
    //---------------------------------------------------------

    /// Knight ability: speed multiplier and duration. Queen reuses this
    /// multiplier while her own ability is active.
    constexpr float KnightSpeedMultiplier = 1.6f;

    constexpr float KnightAbilityDuration = 5.0f;

    /// Rook ability: the shield is really consumed by absorbing one hit,
    /// not by running out of time, but it still gets a long timeout so an
    /// unused shield doesn't linger forever.
    constexpr float RookShieldDuration = 12.0f;

    /// Queen: combines Knight's speed/immunity with Rook's shield for one
    /// shorter duration, per the GDD's "combines multiple abilities for a
    /// short time."
    constexpr float QueenAbilityDuration = 6.0f;

    /// Bishop: how many nearby hazards its ability removes.
    constexpr int BishopRemovalCount = 2;

    /// Bishop clears hazards immediately, but its model lingers briefly so
    /// the player can see which ability just fired.
    constexpr float BishopVisualLingerDuration = 1.0f;

    /// How long a fireball's residual fire patch lingers after impact,
    /// per the GDD's "leave fire behind that deals continuous damage."
    constexpr float FireballBurnDuration = 2.0f;

    /// How close the pawn needs to be to a collectible ally to pick it up.
    constexpr float CollectiblePickupRadius = 0.6f;

    //---------------------------------------------------------
    // Jumping (GDD section 4: Fence/Rock/Palisade "can be jumped over")
    //
    // NOTE(Ayub): the GDD describes the effect (some obstacles can be
    // jumped over) but not the numbers. Placeholder tuning -- adjust
    // freely once Kaung's collision has a real height check to clear.
    //---------------------------------------------------------

    /// Upward speed the instant Space is pressed.
    constexpr float JumpInitialVelocity = 4.2f;

    /// Downward acceleration applied while airborne. Deliberately snappier
    /// than real gravity for an arcade feel.
    constexpr float JumpGravity = 13.0f;

    //---------------------------------------------------------
    // Interaction (E)
    //---------------------------------------------------------

    /// How close the pawn needs to be to a door/gate for E to target it
    /// instead of falling back to the banked ability.
    constexpr float InteractRadius = 1.6f;

    /// How fast a door swings open, in degrees per second.
    constexpr float DoorOpenSpeed = 90.0f;

    /// Both King's Cage leaves use this shared opening-angle magnitude with
    /// opposite signs. Matches the checkpoint gate's 90-degree convention.
    constexpr float KingsCageMaxDoorAngle = 90.0f;


    // Collision Configuration
    //
    //---------------------------------------------------------
    
    // Time after taking damage before the pawn can be damaged again
    constexpr float DamageCooldown = 0.5f;

    // How far the pawn is knocked back when hit
    constexpr float KnockbackDistance = 1.5f;

    /// Slow effect duration when walking through mud/bushes
    constexpr float MudSlowDuration = 2.0f;
    constexpr float MudSlowAmount = 0.5f;

    /// Fireball burn radius and damage
    constexpr float FireballBurnRadius = 0.6f;
    constexpr float FireballBurnDamagePerTick = 0.2f;

    /// Lightning strike
    constexpr float LightningStrikeRadius = 0.5f;
    constexpr float LightningDamage = 1.0f;
}

