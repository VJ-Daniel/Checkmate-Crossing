#pragma once

#include <array>

#include <glm.hpp>

#include "PieceMeshFactory.h"

/// What an animated piece is currently doing.
///
/// Deliberately three values. This describes what the body should look like,
/// not what the gameplay is doing - there is no Attacking or Idle here
/// because nothing in the game drives them yet, and a state nothing sets is
/// a state nobody maintains.
enum class PieceAnimationState
{
    None,
    Walking,
    Jumping
};

/// Which body plan a piece has.
///
/// It decides which axis a limb swings about and nothing else. A humanoid
/// faces along Z and swings its legs about X; the horse is laid out along X
/// and swings its legs about Z. Everything else about the two cycles - the
/// phase offsets, the blending, the tuning - is identical, which is why they
/// share one animator instead of having one each.
enum class PieceBodyPlan
{
    Humanoid,
    Quadruped
};

/// Every number worth turning. One place, so tuning the walk never means
/// hunting through the maths for a magic constant.
namespace PieceAnimationTuning
{
    /// Full stride cycles per second at normal walking speed.
    inline constexpr float WalkCyclesPerSecond = 1.35f;

    /// How far a leg swings from the hip, in degrees, at the extremes.
    inline constexpr float LegSwingDegrees = 26.0f;

    /// Arm swing. Smaller than the legs, or the figure looks like it is
    /// marching rather than walking.
    inline constexpr float ArmSwingDegrees = 18.0f;

    /// Vertical bob of the whole figure, in world units. Two bobs per
    /// stride - one per footfall - so this is small on purpose.
    inline constexpr float WalkBobHeight = 0.018f;

    /// Body counter-rotation, which is what stops the torso reading as a
    /// post with legs underneath it.
    inline constexpr float BodyTwistDegrees = 4.0f;

    /// Forward lean while walking.
    inline constexpr float WalkLeanDegrees = 3.0f;

    /// How far a robed figure's skirt sways. It has no legs to stride with,
    /// so this and the bob are the whole walk.
    inline constexpr float RobeSwayDegrees = 7.0f;

    /// The horse's tail, which trails a quarter-cycle behind the body.
    inline constexpr float TailSwayDegrees = 9.0f;

    /// The horse's neck nod.
    inline constexpr float NeckNodDegrees = 5.0f;

    //---------------------------------------------------------
    // Mounted rider. Secondary motion only: the rider already inherits the
    // horse's bounce through the hierarchy, so these are the small extra
    // movements on top of it - too much and he stops looking seated.
    //---------------------------------------------------------

    /// Rock of the rider's torso against the horse's gait.
    inline constexpr float RiderRockDegrees = 3.5f;

    /// Swing of the rider's arms on the reins.
    inline constexpr float RiderArmDegrees = 4.5f;

    /// How far the rider leans forward over a jump.
    inline constexpr float RiderJumpLeanDegrees = 12.0f;

    //---------------------------------------------------------
    // Jump pose. A held pose rather than a cycle: the piece is in the air
    // for however long the gameplay says, and the body simply reads as
    // airborne for all of it.
    //---------------------------------------------------------

    /// Lead leg tucks up in front, trailing leg swings back.
    inline constexpr float JumpLeadLegDegrees = 34.0f;
    inline constexpr float JumpTrailLegDegrees = -22.0f;

    /// Arms come up and back for balance.
    inline constexpr float JumpArmDegrees = -46.0f;

    /// Slight forward pitch of the torso.
    inline constexpr float JumpLeanDegrees = 7.0f;

    /// The horse tucks its front legs up and kicks the rear pair back.
    inline constexpr float JumpFrontLegDegrees = 38.0f;
    inline constexpr float JumpRearLegDegrees = -30.0f;

    /// Head and neck come up over the obstacle.
    inline constexpr float JumpNeckDegrees = -14.0f;

    //---------------------------------------------------------
    // Take-off against hang time.
    //
    // A jump is not one pose. Pushing off, the legs drive down and the body
    // stretches up; past the apex they tuck. Both come from the vertical
    // velocity the movement code already tracks, so the change is continuous
    // and needs no timer of its own to stay in step with the real arc.
    //---------------------------------------------------------

    /// Vertical speed treated as a full-strength launch. Roughly the speed
    /// a jump actually leaves the ground at, so the push reads at its
    /// strongest exactly when the pawn is climbing hardest.
    inline constexpr float LaunchReferenceSpeed = 4.2f;

    /// How far the legs drive down as the figure pushes off.
    inline constexpr float LaunchLegPushDegrees = -16.0f;

    /// Upward stretch of the whole figure during the push, in world units.
    inline constexpr float LaunchStretchHeight = 0.035f;

    /// The horse drives its legs down harder than a figure does.
    inline constexpr float LaunchHorseLegPushDegrees = -24.0f;

    /// Seconds to blend fully into or out of a state. Short enough to feel
    /// responsive, long enough that landing does not snap.
    inline constexpr float BlendSeconds = 0.12f;
}

/// One frame of animation: an angle for every joint, plus an offset for the
/// figure as a whole.
///
/// Rotations are Euler degrees, matching Transform3D, and are always
/// *relative to the rest pose* - a rig applies them to joints that already
/// sit where the model was authored. The offset moves the visual root only;
/// it is never the piece's world position, which is gameplay's to own.
struct PiecePose
{
    std::array<glm::vec3, PieceMeshFactory::PieceJointCount> jointRotation{};

    glm::vec3 rootOffset = glm::vec3(0.0f);

    void Clear();
};

/// Turns movement state into a pose.
///
/// This is pure maths. It holds no meshes, no transforms and no scene graph,
/// and it never reads or writes anything the gameplay owns - it is handed
/// "moving" and "grounded" and hands back angles. That separation is the
/// point: movement can be rewritten without touching the animation, and the
/// animation can be retuned without any risk to movement or collision.
class PieceAnimator
{
public:

    explicit PieceAnimator(
        PieceBodyPlan bodyPlan = PieceBodyPlan::Humanoid);

    void SetBodyPlan(PieceBodyPlan plan);

    PieceBodyPlan GetBodyPlan() const;

    /// Advances the animation.
    ///
    /// speedScale stretches the stride rate with the piece's actual speed,
    /// so a piece under a speed boost does not moonwalk. Pass 1 for a piece
    /// moving at its normal pace.
    /// verticalVelocity separates pushing off from hanging in the air. Zero
    /// is a perfectly acceptable answer for anything that has no jump
    /// physics - the pose simply holds its airborne shape throughout.
    void Update(
        float deltaTime,
        bool isMoving,
        bool isGrounded,
        float speedScale = 1.0f,
        float verticalVelocity = 0.0f);

    PieceAnimationState GetState() const;

    const PiecePose& GetPose() const;

    /// Drops straight back to the rest pose, with no blend. For a respawn,
    /// where easing out of a walk the player never sees is just wrong.
    void Reset();

private:

    void BuildWalkPose(PiecePose& pose, float weight) const;

    /// launch is 1 while driving off the ground and 0 at the apex and
    /// below; the tucked pose is simply what is left over.
    void BuildJumpPose(PiecePose& pose, float weight, float launch) const;

    PieceBodyPlan bodyPlan;

    PieceAnimationState state;

    /// Stride phase in radians, advanced only while the walk has any weight.
    float phase;

    /// How much of each pose is currently mixed in. Blending on these rather
    /// than snapping between states is what makes landing look like landing
    /// instead of a jump cut.
    float walkWeight;

    float jumpWeight;

    PiecePose pose;
};
