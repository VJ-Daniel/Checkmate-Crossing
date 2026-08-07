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

    /// Bishop: how many nearby stationary obstacles its ability clears.
    ///
    /// Stationary only, by design: the Bishop breaks down the battlefield's
    /// props, it does not swat projectiles out of the air. See
    /// IsAbilityClearable in Obstacle.h for exactly which types qualify.
    constexpr int BishopRemovalCount = 2;

    /// How far from the pawn the Bishop's pulse reaches. Obstacles beyond
    /// this are left alone no matter how few were found inside it.
    constexpr float BishopClearRadius = 3.2f;

    /// Bishop clears hazards immediately, but its model lingers briefly so
    /// the player can see which ability just fired.
    constexpr float BishopVisualLingerDuration = 1.0f;

    //---------------------------------------------------------
    // Bishop / Queen clearing effect
    //
    // Deliberately just two animated sprites: one ring showing how far the
    // pulse reached, and one burst on each prop it actually removed. Both
    // are rebuilt per frame from a timer, so swapping them for a particle
    // system later means replacing AppendAbilityPulseSprites and nothing
    // else.
    //---------------------------------------------------------

    /// How long the whole pulse effect is on screen.
    constexpr float AbilityPulseDuration = 0.55f;

    /// Diameter the expanding ring starts and finishes at, in world units.
    /// The end value is twice BishopClearRadius so the ring lands exactly on
    /// the area the ability actually cleared.
    constexpr float AbilityPulseStartDiameter = 0.8f;

    constexpr float AbilityPulseEndDiameter = BishopClearRadius * 2.0f;

    /// Size of the burst drawn over each cleared obstacle.
    constexpr float AbilityClearBurstSize = 1.6f;

    /// How long a cleared obstacle or sheep keeps reacting before it is
    /// gone for good.
    ///
    /// It stops colliding the instant the ability fires, so the path opens
    /// immediately; this is purely the beat in which it shrinks and fades,
    /// which is what makes it read as "the Bishop destroyed that" rather
    /// than as scenery blinking out.
    constexpr float AbilityDeathReactionDuration = 0.45f;

    /// How far the dying prop shrinks by the end of its reaction, and how
    /// far it sinks into the ground as it goes.
    constexpr float AbilityDeathEndScale = 0.15f;

    constexpr float AbilityDeathSinkDistance = 0.35f;

    //---------------------------------------------------------
    // Fireball (GDD section 2)
    //
    // Launched from the side of the field and lobbed along a lane on a
    // parabolic arc. Every number the trajectory needs is here: the arc is
    // solved mathematically in MovingHazard::UpdateArcProjectile, with no
    // physics engine involved.
    //---------------------------------------------------------

    /// How long one fireball is in the air, start to impact.
    ///
    /// The exposed knob, rather than a speed: the arc is interpolated over
    /// normalised time, so duration is what the trajectory actually reads.
    /// Effective speed is FireballTravelDistance / this -- currently about
    /// 5 world units per second.
    constexpr float FireballTravelDuration = 1.4f;

    /// Peak height of the arc above the lane surface, at the midpoint.
    constexpr float FireballArcHeight = 2.4f;

    /// How far along the lane a fireball travels before it lands. Kept
    /// shorter than the field is wide so it comes down inside the lane
    /// rather than sailing off the far edge.
    constexpr float FireballTravelDistance = 7.0f;

    /// How far outside the playable half-width a fireball is launched from,
    /// so it enters the frame already in flight.
    constexpr float FireballSpawnMargin = 1.0f;

    /// Cooldown between launches on a single lane.
    constexpr float FireballSpawnInterval = 4.0f;

    /// Radius of the projectile's own hit volume while it is airborne. The
    /// fireball is meshless (it is drawn as a sprite), so its collision
    /// cannot be derived from a model and is stated here instead.
    constexpr float FireballHitRadius = 0.45f;

    constexpr float FireballDamage = 1.0f;

    constexpr float FireballKnockback = 2.0f;

    //---------------------------------------------------------
    // Spear (GDD section 2)
    //
    // "Curved projectiles that create a small danger area near their path or
    // landing point." Thrown in from one side on an arc and leaving broken
    // ground where it strikes - deliberately not the arrow's straight sweep,
    // which is what it used to be.
    //
    // It shares the fireball's ArcProjectile pattern but reads differently:
    // flatter, faster, and its danger is the landing rather than a lingering
    // burn.
    //---------------------------------------------------------

    /// How long one spear is in the air.
    constexpr float SpearTravelDuration = 1.05f;

    /// Peak height of the throw. Lower than the fireball's lob - a spear is
    /// hurled at the ground, not lobbed over it.
    constexpr float SpearArcHeight = 1.5f;

    /// How far along the lane a spear travels before it lands.
    constexpr float SpearTravelDistance = 6.0f;

    /// How far outside the playable width it is thrown from.
    constexpr float SpearSpawnMargin = 0.8f;

    /// Radius of the spear's own hit volume while it is in the air.
    constexpr float SpearHitRadius = 0.30f;

    constexpr float SpearDamage = 1.0f;

    constexpr float SpearKnockback = 1.6f;

    //---------------------------------------------------------
    // The broken ground a landed spear leaves
    //---------------------------------------------------------

    /// How long the impact stays dangerous. Short: this is a spot to be
    /// driven off, not an area to be locked out of.
    constexpr float SpearImpactDuration = 1.1f;

    /// Radius of that danger area, and of the ring drawn to show it. One
    /// number for both, so what is drawn is what hurts.
    constexpr float SpearImpactRadius = 0.75f;

    constexpr float SpearImpactDamage = 1.0f;

    /// Opacity of the impact ring, and of the marker that shows where the
    /// spear is going to land while it is still in the air.
    constexpr float SpearImpactRingOpacity = 0.60f;

    constexpr float SpearTelegraphOpacity = 0.55f;

    //---------------------------------------------------------
    // Floor fire left behind by a fireball impact
    //---------------------------------------------------------

    /// How long a fireball's residual fire patch lingers after impact,
    /// per the GDD's "leave fire behind that deals continuous damage."
    constexpr float FireballBurnDuration = 2.0f;

    /// How far from the patch centre the fire burns.
    ///
    /// Raised from 0.6 so the damaging area matches the flame the player can
    /// actually see. At 0.6 the ring drawn from this number was narrower than
    /// the flame's own base, so it was completely hidden underneath it - the
    /// marker has to clear the art for "what you see is what burns you" to
    /// mean anything.
    constexpr float FloorFireRadius = 0.9f;

    /// Damage per tick while standing in it.
    constexpr float FloorFireDamage = 0.5f;

    /// Seconds between burn ticks.
    ///
    /// The pawn is burned the moment it steps in and then every interval it
    /// stays, with the count restarting from scratch if it leaves and comes
    /// back. Deliberately independent of DamageCooldown, which is shorter
    /// and would otherwise set the burn rate.
    constexpr float FloorFireDamageInterval = 1.0f;

    /// On-screen size of the camera-facing flame billboard.
    ///
    /// Kept close to the ring's diameter (2 * FloorFireRadius) so the flame
    /// does not visibly overhang the area that actually burns.
    constexpr float FloorFireSpriteSize = 1.9f;

    /// How many CraftPix flame frames the floor fire cycles through.
    constexpr int FloorFireFrameCount = 8;

    /// Flame animation rate. The frames are played out and back rather than
    /// looped end-to-start: the sequence grows from a small flame to a full
    /// one, so restarting it would snap large-to-small every cycle, while
    /// bouncing reads as the fire breathing.
    constexpr float FloorFireFramesPerSecond = 14.0f;

    /// Radius of the red hazard ring drawn under a floor fire.
    ///
    /// Deliberately the damage radius itself: the ring is a promise about
    /// where the fire hurts, so drawing it any other size would be a lie.
    constexpr float FloorFireRingRadius = FloorFireRadius;

    /// Opacity of that ring. Enough to read against the grass, low enough
    /// to see the lane markings under it.
    constexpr float FloorFireRingOpacity = 0.55f;

    /// How long the impact flash lasts when a fireball lands.
    constexpr float FireballImpactFlashDuration = 0.22f;

    /// Diameter the flash expands from and to.
    constexpr float FireballImpactFlashStartSize = 0.6f;

    constexpr float FireballImpactFlashEndSize = 3.0f;

    /// Ground shadow cast by the projectile in flight.
    ///
    /// Size and opacity both shrink with altitude, which is the cue that
    /// tells the player how close to landing it is, and the position tracks
    /// only X/Z so the shadow stays on the floor.
    constexpr float FireballShadowSize = 1.0f;

    constexpr float FireballShadowOpacity = 0.38f;

    /// How much of the shadow's size and strength is lost at the apex.
    constexpr float FireballShadowHeightFalloff = 0.55f;

    //---------------------------------------------------------
    // Lightning (GDD section 2)
    //
    // Two phases: a warning marker the player can walk out of, then a
    // strike that only damages whoever is still inside. Any number of these
    // can run at once - each is an independent hazard.
    //---------------------------------------------------------

    /// How long the ground marker shows before the bolt lands. This is the
    /// player's whole reaction window, so it is the first number to raise
    /// if the strike feels unfair.
    constexpr float LightningWarningDuration = 2.0f;

    /// How long the bolt stays on screen after it lands. Damage is applied
    /// once, at the start of this window, not across it.
    constexpr float LightningStrikeDuration = 0.6f;

    /// Radius of the marked area. Used for the escape test, the damage and
    /// the warning decal alike, so what is drawn is exactly what is
    /// dangerous and exactly what has to be left to be safe.
    constexpr float LightningStrikeRadius = 1.4f;

    /// How often a lightning zone re-arms. Each activation marks its area,
    /// counts down, strikes and finishes, so this is the gap between one
    /// finishing and the next appearing.
    constexpr float LightningSpawnInterval = 4.0f;

    /// How long the warning marker takes to complete one flash cycle. Short
    /// enough to read as urgent rather than as a slow pulse.
    constexpr float LightningWarningFlashPeriod = 0.32f;

    constexpr float LightningDamage = 1.0f;

    constexpr float LightningKnockback = 0.5f;

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

    // Fireball burn and lightning strike tuning used to be repeated here.
    // They now live with the rest of each hazard's numbers in the Fireball,
    // floor fire and Lightning sections above -- this copy was never read
    // by anything, while the collision pass hardcoded its own values, so
    // changing these had no effect at all.

    //---------------------------------------------------------
    // Sheep AI (Ayub)
    //
    // The sheep grazes until the pawn comes within detection range, then
    // follows while keeping a standoff distance rather than overlapping.
    // Detection is deliberately well above the follow distance, or it would
    // notice and stop in the same instant and never take a step.
    //---------------------------------------------------------

    /// How close the pawn must come before a sheep starts following.
    constexpr float SheepDetectionRange = 5.0f;

    /// The gap a following sheep tries to keep from the pawn.
    ///
    /// Zero: it closes all the way and stays in contact, shoving until the
    /// player works free. It used to hold 1.5 units, which meant it stopped
    /// short, touched once and then politely waited - the hazard barged you
    /// once and gave up.
    constexpr float SheepFollowDistance = 0.0f;

    /// How hard a sheep in contact shoves, per frame of contact.
    ///
    /// This is a sustained push rather than the one-off impulse other
    /// hazards deal, so it is much smaller than KnockbackDistance: it is
    /// added on top of the player's own movement every frame they stay in
    /// contact, and is meant to lose them ground while they walk out of it,
    /// not to fling them.
    constexpr float SheepPushStrength = 2.6f;

    //---------------------------------------------------------
    // Pawn health
    //---------------------------------------------------------

    /// Pawn's maximum HP. Single source of truth so the HUD's health pips
    /// and Pawn's own starting health can't drift apart.
    constexpr float MaxPawnHealth = 5.0f;

    //---------------------------------------------------------
    // Game over
    //
    // Death already respawns the pawn at its most recent checkpoint (or
    // the level start, if none yet) -- see Pawn::TakeDamage/Respawn. This
    // is the harsher reset layered on top: too many deaths sends the
    // player all the way back to the level's initial spawn point and
    // clears checkpoint progress, not just the pawn's own position.
    //---------------------------------------------------------

    constexpr int MaxDeathsBeforeGameOver = 3;

    /// How long the Game Over banner stays on screen. Gameplay is not
    /// paused during this window -- the reset already happened the instant
    /// the death count crossed the threshold, this is purely feedback.
    constexpr float GameOverBannerDuration = 2.0f;

    //---------------------------------------------------------
    // Victory
    //
    // Triggers once, the instant the pawn reaches the King inside the
    // (opened) King's Cage. A one-shot terminal state, unlike Game Over --
    // nothing resets and it never fires twice in one run.
    //---------------------------------------------------------

    /// How close the pawn needs to be to the King to count as "got him".
    constexpr float KingRescueRadius = 1.0f;

    /// Same duration as the Game Over banner, for the same reason: gameplay
    /// isn't paused while it's up, this is purely feedback.
    constexpr float VictoryBannerDuration = 2.0f;

    //---------------------------------------------------------
    // Controls screen
    //
    // A brief key-icon reminder shown for the first few seconds of a run.
    // No text rendering exists, so this leans entirely on the keycap art
    // (WASD/Space/E) being self-explanatory.
    //---------------------------------------------------------

    constexpr float ControlsScreenDuration = 4.0f;

    //---------------------------------------------------------
    // HUD
    //
    // Pixel layout for the always-on-screen HUD. All Screen-mode sprites
    // are positioned by their centre, origin at the window's top-left, so
    // these are expressed as margins from a corner/edge plus element
    // sizes -- see Game::AppendHudSprites for how they turn into actual
    // positions at the current window size.
    //---------------------------------------------------------

    constexpr float HudMargin = 20.0f;

    /// Health pips (top-left).
    constexpr float HudHealthPipSize = 28.0f;
    constexpr float HudHealthPipSpacing = 8.0f;

    /// Checkpoint pips (top-centre).
    constexpr float HudCheckpointPipSize = 24.0f;
    constexpr float HudCheckpointPipSpacing = 12.0f;

    /// Ability icon + duration bar (top-right).
    constexpr float HudAbilityIconSize = 48.0f;
    constexpr float HudAbilityBarWidth = 64.0f;
    constexpr float HudAbilityBarHeight = 10.0f;
    constexpr float HudAbilityBarGap = 8.0f;

    /// Current piece icon (bottom-centre).
    constexpr float HudPieceIconSize = 56.0f;

    /// Interact prompt (centred, above the current piece icon). Shown only
    /// while the pawn is within InteractRadius of a checkpoint gate or the
    /// King's Cage.
    constexpr float HudInteractPromptSize = 40.0f;
    constexpr float HudInteractPromptGap = 12.0f;

    /// Game Over banner (screen-centred) and the dimming overlay behind it.
    /// The banner art is a square ornate frame, so this stays square to
    /// avoid stretching its border out of proportion.
    constexpr float HudGameOverBannerSize = 240.0f;
    inline const glm::vec3 HudGameOverOverlayTint = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr float HudGameOverOverlayOpacity = 0.55f;

    /// Tints the (currently white) frame art a somber red for the "defeat"
    /// mood without baking a color into the source asset.
    inline const glm::vec3 HudGameOverBannerTint = glm::vec3(0.75f, 0.20f, 0.20f);

    /// "GAME OVER" text, rendered once offline (MedievalSharp, gold fill,
    /// dark stroke) rather than tinted -- it needs to stay legible on top
    /// of the red frame, not share its color. Sized to match the source
    /// PNG's ~4.4:1 aspect so it doesn't stretch.
    constexpr float HudGameOverTextWidth = 200.0f;
    constexpr float HudGameOverTextHeight = 46.0f;

    /// Victory banner (screen-centred) and its text -- same construction as
    /// the Game Over banner, different frame art/tint/text so the two never
    /// read as the same event.
    constexpr float HudVictoryBannerSize = 240.0f;
    inline const glm::vec3 HudVictoryBannerTint = glm::vec3(0.85f, 0.68f, 0.15f);
    constexpr float HudVictoryTextWidth = 190.0f;
    constexpr float HudVictoryTextHeight = 56.0f;

    /// Controls screen: a row of keycap icons (WASD diamond, then Space,
    /// then E), centred on screen with a light dimming overlay -- lighter
    /// than the Game Over/Victory overlay since this isn't a dramatic
    /// event, just a reminder shown over the start of normal gameplay.
    constexpr float HudControlsKeySize = 48.0f;
    constexpr float HudControlsKeyGap = 8.0f;
    constexpr float HudControlsGroupGap = 24.0f;
    inline const glm::vec3 HudControlsOverlayTint = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr float HudControlsOverlayOpacity = 0.35f;

    /// Shared pip/bar tints, multiplied onto the white placeholder texture.
    inline const glm::vec3 HudFilledTint = glm::vec3(0.85f, 0.68f, 0.15f);
    inline const glm::vec3 HudEmptyTint = glm::vec3(0.30f, 0.30f, 0.33f);
    inline const glm::vec3 HudBarBackTint = glm::vec3(0.15f, 0.15f, 0.18f);

    /// Health pips read better as hearts than gold, so they get their own
    /// filled tint instead of the shared HudFilledTint.
    inline const glm::vec3 HudHealthFilledTint = glm::vec3(0.82f, 0.16f, 0.16f);
}

