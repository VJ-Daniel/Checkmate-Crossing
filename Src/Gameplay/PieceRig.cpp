/*
    ============================================================
    Checkmate Crossing - Piece Rig

    The scene-graph half of the animation system: a chess piece as a
    tree of parts, each rotating about its own joint.

    Everything here is transforms. What the angles should be is
    PieceAnimator's problem, and where the piece is in the world is the
    gameplay's - the rig only assembles the two.
    ============================================================
*/

#include "PieceRig.h"

using PieceMeshFactory::PieceJoint;
using PieceMeshFactory::PieceJointCount;
using PieceMeshFactory::PieceRigModel;
using PieceMeshFactory::PieceRigPartModel;

PieceRig::PieceRig()
    : groundPosition(0.0f),
    headingDegrees(0.0f),
    scale(1.0f)
{
}

PieceJoint PieceRig::GetParent(PieceJoint joint)
{
    switch (joint)
    {
        // The arms follow the torso, so a body lean or twist carries them
        // with it. The horse's neck and tail do the same.
    case PieceJoint::LeftArm:
    case PieceJoint::RightArm:
    case PieceJoint::Head:
    case PieceJoint::Tail:
        return PieceJoint::Body;

        // Legs and skirt hang off the root instead. Parented to the body
        // they would inherit its lean and twist, and the feet would slide
        // sideways every time the torso turned.
    default:
        return PieceJoint::Root;
    }
}

void PieceRig::Build(const PieceRigModel& model)
{
    joints = {};
    parts.clear();

    if (!model.valid)
        return;

    // The root carries no geometry. It exists so the whole figure has one
    // thing to be placed and bobbed by.
    joints[static_cast<int>(PieceJoint::Root)] =
        std::make_shared<SceneNode>();

    for (int index = 0; index < PieceJointCount; ++index)
    {
        const PieceRigPartModel& part = model.parts[index];

        if (!part.mesh)
            continue;

        auto node = std::make_shared<SceneNode>();

        node->SetMesh(part.mesh);

        // Flat white, so the mesh's own vertex colours come through
        // unchanged - the same contract the props and the gate use.
        node->SetColor(glm::vec4(1.0f));

        joints[index] = node;
    }

    // Link the tree, and give each node the rest offset that puts its joint
    // where the model was authored. A child's offset is the gap between its
    // own pivot and its parent's, which is what makes a chain of rotations
    // come out in the right place.
    for (int index = 0; index < PieceJointCount; ++index)
    {
        const auto& node = joints[index];

        if (!node || index == static_cast<int>(PieceJoint::Root))
            continue;

        const auto joint = static_cast<PieceJoint>(index);

        PieceJoint parentJoint = GetParent(joint);

        // A joint whose parent this model does not have falls back to the
        // root, so a partial rig - the horse has no arms, the bishop no
        // legs - still assembles instead of dropping parts on the floor.
        if (!joints[static_cast<int>(parentJoint)])
            parentJoint = PieceJoint::Root;

        const auto& parentNode = joints[static_cast<int>(parentJoint)];

        const glm::vec3 parentPivot =
            (parentJoint == PieceJoint::Root)
            ? glm::vec3(0.0f)
            : model.parts[static_cast<int>(parentJoint)].pivot;

        node->GetTransform().SetPosition(
            model.parts[index].pivot - parentPivot);

        parentNode->AddChild(node);

        parts.push_back(node);
    }
}

bool PieceRig::IsBuilt() const
{
    return joints[static_cast<int>(PieceJoint::Root)] != nullptr;
}

void PieceRig::SetGroundPosition(const glm::vec3& position)
{
    groundPosition = position;
}

void PieceRig::SetHeadingDegrees(float degrees)
{
    headingDegrees = degrees;
}

void PieceRig::SetScale(float uniformScale)
{
    scale = uniformScale;
}

void PieceRig::ApplyPose(const PiecePose& pose)
{
    const auto& root = joints[static_cast<int>(PieceJoint::Root)];

    if (!root)
        return;

    // The root is rebuilt from the ground position every frame rather than
    // nudged, so the pose's bob cannot accumulate into a figure that slowly
    // rises off the ground.
    root->GetTransform().SetPosition(
        groundPosition + pose.rootOffset * scale);

    root->GetTransform().SetRotation(0.0f, headingDegrees, 0.0f);
    root->GetTransform().SetScale(scale, scale, scale);

    for (int index = 0; index < PieceJointCount; ++index)
    {
        const auto& node = joints[index];

        if (!node || index == static_cast<int>(PieceJoint::Root))
            continue;

        node->GetTransform().SetRotation(pose.jointRotation[index]);
    }

    root->UpdateWorldTransforms();
}

const std::vector<std::shared_ptr<SceneNode>>& PieceRig::GetParts() const
{
    return parts;
}

SceneNode* PieceRig::GetJoint(PieceJoint joint)
{
    return joints[static_cast<int>(joint)].get();
}
