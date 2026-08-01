#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "WorldObject.h"

/// A world object that can carry children, so a model made of several meshes
/// can be posed as a hierarchy instead of as a pile of loose transforms.
///
/// Everything else in this project bakes a model into one mesh precisely so
/// no scene graph is needed, and for scenery that is still the right answer.
/// An animated figure is the case that cannot work that way: a forearm has to
/// follow its upper arm, which follows the shoulder, which follows the body.
/// Writing those chains out by hand is how a rig ends up with the sword
/// hovering a few centimetres from the hand.
///
/// The tree is deliberately thin. A node owns a local transform - the one
/// animation writes to - and caches the world matrix it was last given.
/// Nothing here knows what a joint is or what walking looks like; those live
/// in PieceRig and PieceAnimator respectively.
class SceneNode : public WorldObject
{
public:

    SceneNode();

    /// Attaches a child. The child's transform is from then on read as being
    /// relative to this node.
    void AddChild(std::shared_ptr<SceneNode> child);

    const std::vector<std::shared_ptr<SceneNode>>& GetChildren() const;

    /// Recomputes this node's world matrix and pushes the result down the
    /// tree. Call once on the root after posing, and before drawing.
    ///
    /// Kept as an explicit pass rather than resolved lazily per node: a
    /// lazy chain walks back to the root for every part it draws, and this
    /// way each node is visited exactly once whatever the tree looks like.
    void UpdateWorldTransforms(
        const glm::mat4& parentWorld = glm::mat4(1.0f));

    /// The matrix the renderer should draw this node with.
    ///
    /// Valid as of the last UpdateWorldTransforms; a node that has never
    /// been through one draws with its local transform, which is exactly
    /// what a parentless node's world transform is anyway.
    glm::mat4 GetWorldMatrix() const override;

private:

    std::vector<std::shared_ptr<SceneNode>> children;

    glm::mat4 worldMatrix;
};
