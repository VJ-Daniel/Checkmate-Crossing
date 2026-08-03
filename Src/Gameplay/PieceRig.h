#pragma once

#include <array>
#include <memory>
#include <vector>

#include <glm.hpp>

#include "PieceAnimator.h"
#include "PieceMeshFactory.h"
#include "SceneNode.h"

/// One animated chess piece, as a hierarchy of parts.
///
/// The rig owns the scene nodes and nothing else: it knows where the joints
/// are and which hangs off which, and it can apply a PiecePose. It has no
/// opinion about walking, and no idea what the piece is doing - that all
/// lives in PieceAnimator, on the other side of a plain struct of angles.
///
/// A rig is placed the same way any other model is: give it the point of
/// ground it stands on and a heading, and it puts its root there. Everything
/// inside is relative to that, so the gameplay never has to know a rig is
/// made of eleven meshes rather than one.
class PieceRig
{
public:

    PieceRig();

    /// Builds the node tree from a rig model. Needs a live GL context, the
    /// same as every other model.
    void Build(const PieceMeshFactory::PieceRigModel& model);

    bool IsBuilt() const;

    /// Stands the rig on a point of ground, facing a heading in degrees.
    void SetGroundPosition(const glm::vec3& groundPosition);

    void SetHeadingDegrees(float degrees);

    void SetScale(float uniformScale);

    /// Writes a pose onto the joints and refreshes the world transforms.
    ///
    /// Rest pose plus pose, never pose alone: a joint's node already carries
    /// the offset that puts it where the model was authored, and animation
    /// only ever adds rotation on top of that.
    void ApplyPose(const PiecePose& pose);

    /// Every part that has something to draw, in no particular order - the
    /// depth buffer sorts them. Flat rather than a tree walk because the
    /// tree never changes shape once built.
    const std::vector<std::shared_ptr<SceneNode>>& GetParts() const;

    /// The node a joint is carried by, or null if this model has no such
    /// joint. Exposed for anything that needs to hang an object off a piece
    /// later - a collected ally, a held item - without going through a pose.
    SceneNode* GetJoint(PieceMeshFactory::PieceJoint joint);

private:

    std::array<
        std::shared_ptr<SceneNode>,
        PieceMeshFactory::PieceJointCount> joints;

    /// Flat list of the parts worth drawing, built once.
    std::vector<std::shared_ptr<SceneNode>> parts;

    /// Where the whole rig stands. Held separately from the root node's
    /// transform because the pose adds an offset on top of it every frame,
    /// and the two must not accumulate.
    glm::vec3 groundPosition;

    float headingDegrees;

    float scale;
};
