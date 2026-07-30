/*
    ============================================================
    Checkmate Crossing - World Object

    Defines the common composition used by every 3D battlefield entity:
    Transform3D, Mesh, colour, visibility and active state.
    ============================================================
*/

#include "WorldObject.h"

WorldObject::WorldObject()
    : color(1.0f),
    visible(true),
    active(true)
{
}

WorldObject::WorldObject(
    std::shared_ptr<Mesh> mesh,
    const glm::vec4& color)
    : mesh(std::move(mesh)),
    color(color),
    visible(true),
    active(true)
{
}

void WorldObject::Initialize()
{
}

void WorldObject::Update(float deltaTime)
{
    (void)deltaTime;
}

Transform3D& WorldObject::GetTransform()
{
    return transform;
}

const Transform3D& WorldObject::GetTransform() const
{
    return transform;
}

void WorldObject::SetMesh(std::shared_ptr<Mesh> mesh)
{
    this->mesh = std::move(mesh);
}

const std::shared_ptr<Mesh>& WorldObject::GetMesh() const
{
    return mesh;
}

void WorldObject::SetColor(const glm::vec4& color)
{
    this->color = color;
}

const glm::vec4& WorldObject::GetColor() const
{
    return color;
}

void WorldObject::SetVisible(bool visible)
{
    this->visible = visible;
}

bool WorldObject::IsVisible() const
{
    return visible;
}

void WorldObject::SetActive(bool active)
{
    this->active = active;
}

bool WorldObject::IsActive() const
{
    return active;
}
