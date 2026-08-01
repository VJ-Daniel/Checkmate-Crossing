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
    float speedScale)
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
        BuildJumpPose(pose, jumpWeight);
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
    }

    // Two bobs per stride, one per footfall. abs() of a sine at the stride
    // rate gives exactly that without a second phase to keep in step.
    target.rootOffset.y +=
        std::fabs(std::sin(phase)) * WalkBobHeight * weight;
}

void PieceAnimator::BuildJumpPose(PiecePose& target, float weight) const
{
    using namespace PieceMeshFactory;
    using namespace PieceAnimationTuning;

    const int axis = SwingAxis;

    if (bodyPlan == PieceBodyPlan::Humanoid)
    {
        // One leg tucked up in front and one trailing, rather than both
        // together: a symmetrical tuck reads as sitting down in mid-air.
        AddSwing(target, PieceJoint::LeftLeg, axis,
            JumpLeadLegDegrees * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            JumpTrailLegDegrees * weight);

        AddSwing(target, PieceJoint::LeftArm, axis,
            JumpArmDegrees * weight);

        AddSwing(target, PieceJoint::RightArm, axis,
            JumpArmDegrees * weight);

        // The robe swings back from the lift, which is the closest a single
        // skirt gets to tucked legs.
        AddSwing(target, PieceJoint::Robe, axis,
            JumpTrailLegDegrees * 0.5f * weight);

        AddSwing(target, PieceJoint::Body, axis,
            JumpLeanDegrees * weight);
    }
    else
    {
        AddSwing(target, PieceJoint::LeftLeg, axis,
            JumpFrontLegDegrees * weight);

        AddSwing(target, PieceJoint::RightLeg, axis,
            JumpFrontLegDegrees * weight);

        AddSwing(target, PieceJoint::RearLeftLeg, axis,
            JumpRearLegDegrees * weight);

        AddSwing(target, PieceJoint::RearRightLeg, axis,
            JumpRearLegDegrees * weight);

        AddSwing(target, PieceJoint::Head, axis,
            JumpNeckDegrees * weight);

        AddSwing(target, PieceJoint::Tail, axis,
            JumpRearLegDegrees * 0.4f * weight);
    }

    // No root offset. How high the piece actually goes is the gameplay's
    // to decide; the animator only changes what the body looks like on the
    // way up and down.
}
