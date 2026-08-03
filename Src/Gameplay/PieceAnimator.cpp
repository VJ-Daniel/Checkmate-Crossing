/*
    ============================================================
    Checkmate Crossing - Piece Animator

    Turns "moving" and "grounded" into a set of joint angles.

    No meshes, no transforms, no scene graph and nothing the gameplay
    owns: the animator is handed two booleans and a timestep, and hands
    back a pose. Everything visual happens in PieceRig, and everything
    about where the piece actually is happens in Pawn.
    ============================================================
*/

#include "PieceAnimator.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    /// Eases a weight toward a target at a fixed rate, framerate independent.
    float MoveToward(float value, float target, float maxDelta)
    {
        if (value < target)
            return std::min(value + maxDelta, target);

        return std::max(value - maxDelta, target);
    }

    /// The Euler component a limb swings on.
    ///
    /// X, for every piece. Each model is authored facing +Z, so a limb
    /// swinging forward and back turns about X whatever the body plan is.
    ///
    /// This used to depend on the body plan, because the horse was the one
    /// model laid out along X instead. Normalising the horse to +Z removed
    /// that difference, and with it a whole class of "which way does this
    /// one face" question from the animation code.
    constexpr int SwingAxis = 0;

    /// The axis a body twists about while walking - always vertical.
    constexpr int TwistAxis = 1;

    void AddSwing(
        PiecePose& pose,
        PieceMeshFactory::PieceJoint joint,
        int axis,
        float degrees)
    {
        pose.jointRotation[static_cast<int>(joint)][axis] += degrees;
    }
}

void PiecePose::Clear()
{
    for (glm::vec3& rotation : jointRotation)
        rotation = glm::vec3(0.0f);

    rootOffset = glm::vec3(0.0f);
}

PieceAnimator::PieceAnimator(PieceBodyPlan bodyPlan)
    : bodyPlan(bodyPlan),
    state(PieceAnimationState::None),
    phase(0.0f),
    walkWeight(0.0f),
    jumpWeight(0.0f)
{
}

void PieceAnimator::SetBodyPlan(PieceBodyPlan plan)
{
    bodyPlan = plan;
}

PieceBodyPlan PieceAnimator::GetBodyPlan() const
{
    return bodyPlan;
}

PieceAnimationState PieceAnimator::GetState() const
{
    return state;
}

const PiecePose& PieceAnimator::GetPose() const
{
    return pose;
}

void PieceAnimator::Reset()
{
    state = PieceAnimationState::None;
    phase = 0.0f;
    walkWeight = 0.0f;
    jumpWeight = 0.0f;

    pose.Clear();
}

void PieceAnimator::Update(
    float deltaTime,
    bool isMoving,
    bool isGrounded,
    float speedScale,
    float verticalVelocity)
{
    // Airborne wins over moving: a piece that jumps while running is
    // jumping, and reads as jumping, until it lands.
    if (!isGrounded)
        state = PieceAnimationState::Jumping;
    else if (isMoving)
        state = PieceAnimationState::Walking;
    else
        state = PieceAnimationState::None;

    const float blendRate = (PieceAnimationTuning::BlendSeconds > 0.0f)
        ? deltaTime / PieceAnimationTuning::BlendSeconds
        : 1.0f;

    walkWeight = MoveToward(
        walkWeight,
        (state == PieceAnimationState::Walking) ? 1.0f : 0.0f,
        blendRate);

    jumpWeight = MoveToward(
        jumpWeight,
        (state == PieceAnimationState::Jumping) ? 1.0f : 0.0f,
        blendRate);

    // The stride keeps turning while the walk is blending out, so a figure
    // that stops mid-step finishes the step instead of freezing on one leg.
    if (walkWeight > 0.0f)
    {
        phase += deltaTime *
            PieceAnimationTuning::WalkCyclesPerSecond *
            std::max(speedScale, 0.0f) *
            2.0f * Pi;

        // Wrapped so the phase cannot grow until float precision starts to
        // coarsen the cycle - which takes a while, but shows up as a walk
        // that visibly stutters after a long session.
        if (phase > 2.0f * Pi)
            phase -= 2.0f * Pi;
    }

    pose.Clear();

    // The two poses are added rather than cross-faded. Their angles are
    // small and they are never both at full weight, so summing them gives a
    // clean hand-off without a second set of blend rules.
    if (walkWeight > 0.0f)
        BuildWalkPose(pose, walkWeight);

    if (jumpWeight > 0.0f)
    {
        // Climbing hard reads as a launch; at the apex and on the way down
        // it reads as hanging. Taken from the real vertical speed, so the
        // pose follows the actual arc instead of a guess at its timing.
        const float launch = std::min(
            std::max(
                verticalVelocity / PieceAnimationTuning::LaunchReferenceSpeed,
                0.0f),
            1.0f);

        BuildJumpPose(pose, jumpWeight, launch);
    }
}

void PieceAnimator::BuildWalkPose(PiecePose& target, float weight) const
{
    using namespace PieceMeshFactory;
    using namespace PieceAnimationTuning;

    const int axis = SwingAxis;

    const float swing = std::sin(phase);
    const float counterSwing = std::sin(phase + Pi);

    if (bodyPlan == PieceBodyPlan::Humanoid)
    {
        // Legs alternate, and each arm swings against the leg on its own
        // side - which is what walking actually is, and what stops the
        // figure looking like it is skiing.
        AddSwing(target, PieceJoint::LeftLeg, axis,
            swing * LegSwingDegrees * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            counterSwing * LegSwingDegrees * weight);

        AddSwing(target, PieceJoint::LeftArm, axis,
            counterSwing * ArmSwingDegrees * weight);

        AddSwing(target, PieceJoint::RightArm, axis,
            swing * ArmSwingDegrees * weight);

        // A robed figure has no legs to stride with, so its skirt takes the
        // swing instead. Rocking it side to side rather than front to back
        // is what reads as a robe moving with the walk.
        AddSwing(target, PieceJoint::Robe, 2,
            swing * RobeSwayDegrees * weight);

        AddSwing(target, PieceJoint::Body, TwistAxis,
            counterSwing * BodyTwistDegrees * weight);

        AddSwing(target, PieceJoint::Body, axis,
            WalkLeanDegrees * weight);
    }
    else
    {
        // Diagonal pairs, the way a horse actually walks: front-left moves
        // with rear-right. Giving all four the same phase reads as a
        // rocking horse, and giving them four different phases reads as a
        // limp - the diagonal pairing is the one that looks like a horse.
        AddSwing(target, PieceJoint::LeftLeg, axis,
            swing * LegSwingDegrees * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            counterSwing * LegSwingDegrees * weight);

        AddSwing(target, PieceJoint::RearLeftLeg, axis,
            counterSwing * LegSwingDegrees * weight);

        AddSwing(target, PieceJoint::RearRightLeg, axis,
            swing * LegSwingDegrees * weight);

        // Neck nods at twice the stride rate - once per footfall - and the
        // tail trails a quarter cycle behind the body.
        AddSwing(target, PieceJoint::Head, axis,
            std::sin(phase * 2.0f) * NeckNodDegrees * weight);

        AddSwing(target, PieceJoint::Tail, axis,
            std::sin(phase - Pi * 0.5f) * TailSwayDegrees * weight);

        AddSwing(target, PieceJoint::Body, axis,
            std::sin(phase * 2.0f) * BodyTwistDegrees * 0.5f * weight);

        // A rider, if this model carries one. The Rider joint already hangs
        // off the horse's Body, so the bounce and rock reach him through the
        // hierarchy for free - these are only the secondary movements on top.
        //
        // Written unconditionally: a horse with no rider simply has no such
        // joints, and the rig drops angles for parts that do not exist.
        AddSwing(target, PieceJoint::Rider, axis,
            std::sin(phase * 2.0f + Pi) * RiderRockDegrees * weight);

        AddSwing(target, PieceJoint::LeftArm, axis,
            swing * RiderArmDegrees * weight);

        AddSwing(target, PieceJoint::RightArm, axis,
            counterSwing * RiderArmDegrees * weight);
    }

    // Two bobs per stride, one per footfall. abs() of a sine at the stride
    // rate gives exactly that without a second phase to keep in step.
    target.rootOffset.y +=
        std::fabs(std::sin(phase)) * WalkBobHeight * weight;
}

void PieceAnimator::BuildJumpPose(
    PiecePose& target,
    float weight,
    float launch) const
{
    using namespace PieceMeshFactory;
    using namespace PieceAnimationTuning;

    const int axis = SwingAxis;

    // Whatever is not a launch is hang time. The two shapes are mixed by
    // these rather than switched between, so the body flows from driving off
    // the ground into a tuck instead of snapping at some threshold.
    const float hang = 1.0f - launch;

    if (bodyPlan == PieceBodyPlan::Humanoid)
    {
        // Pushing off, both legs drive down together. At the apex they tuck
        // - one up in front and one trailing, because a symmetrical tuck
        // reads as sitting down in mid-air.
        const float legPush = LaunchLegPushDegrees * launch * weight;

        AddSwing(target, PieceJoint::LeftLeg, axis,
            legPush + JumpLeadLegDegrees * hang * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            legPush + JumpTrailLegDegrees * hang * weight);

        // Arms come up for balance the moment the feet leave the ground and
        // stay there, so there is nothing to blend on this one.
        AddSwing(target, PieceJoint::LeftArm, axis,
            JumpArmDegrees * weight);

        AddSwing(target, PieceJoint::RightArm, axis,
            JumpArmDegrees * weight);

        // The robe swings back from the lift, which is the closest a single
        // skirt gets to tucked legs.
        AddSwing(target, PieceJoint::Robe, axis,
            JumpTrailLegDegrees * 0.5f * hang * weight);

        AddSwing(target, PieceJoint::Body, axis,
            JumpLeanDegrees * hang * weight);

        // Stretch upward on the push only. This moves the visual root, never
        // the piece - the rig rebuilds the root from the ground position
        // every frame, so it cannot accumulate.
        target.rootOffset.y += LaunchStretchHeight * launch * weight;
    }
    else
    {
        const float horsePush = LaunchHorseLegPushDegrees * launch * weight;

        AddSwing(target, PieceJoint::LeftLeg, axis,
            horsePush + JumpFrontLegDegrees * hang * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            horsePush + JumpFrontLegDegrees * hang * weight);

        AddSwing(target, PieceJoint::RearLeftLeg, axis,
            horsePush + JumpRearLegDegrees * hang * weight);

        AddSwing(target, PieceJoint::RearRightLeg, axis,
            horsePush + JumpRearLegDegrees * hang * weight);

        // Head and neck come up hardest over the push, which is what makes
        // the front of the animal rise into the jump.
        AddSwing(target, PieceJoint::Head, axis,
            JumpNeckDegrees * (0.5f + 0.5f * launch) * weight);

        AddSwing(target, PieceJoint::Tail, axis,
            JumpRearLegDegrees * 0.4f * hang * weight);

        // The rider folds forward over the horse's neck on the way up and
        // settles once the arc levels out.
        AddSwing(target, PieceJoint::Rider, axis,
            RiderJumpLeanDegrees * (0.4f + 0.6f * launch) * weight);

        target.rootOffset.y += LaunchStretchHeight * launch * weight;
    }
}
