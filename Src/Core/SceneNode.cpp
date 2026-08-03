/*
    ============================================================
    Checkmate Crossing - Scene Node

    One level of parent/child transform, for the models that are made
    of several meshes rather than one: the animated chess pieces.
    ============================================================
*/

#include "SceneNode.h"

SceneNode::SceneNode()
    : worldMatrix(1.0f)
{
}

void SceneNode::AddChild(std::shared_ptr<SceneNode> child)
{
    if (child)
        children.push_back(std::move(child));
}

const std::vector<std::shared_ptr<SceneNode>>& SceneNode::GetChildren() const
{
    return children;
}

void SceneNode::UpdateWorldTransforms(const glm::mat4& parentWorld)
{
    worldMatrix = parentWorld * transform.GetModelMatrix();

    for (const auto& child : children)
    {
        if (child)
            child->UpdateWorldTransforms(worldMatrix);
    }
}

glm::mat4 SceneNode::GetWorldMatrix() const
{
    return worldMatrix;
}
