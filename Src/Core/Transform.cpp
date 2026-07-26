/*
    ============================================================
    Gangster Survival - Transform
    Author: Leonardo Moura | Date: 2026

    Stores and modifies 2D position, rotation and rendered scale.
    These values are converted into a model matrix by the renderer.
    ============================================================
*/

#include "Transform.h"

Transform::Transform()
    : position(0.0f, 0.0f),
    rotation(0.0f),
    scale(1.0f, 1.0f)
{
}

Transform::Transform(
    const glm::vec2& position,
    float rotation,
    const glm::vec2& scale)
    : position(position),
    rotation(rotation),
    scale(scale)
{
}

void Transform::SetPosition(const glm::vec2& position)
{
    this->position = position;
}

void Transform::SetPosition(float x, float y)
{
    position.x = x;
    position.y = y;
}

const glm::vec2& Transform::GetPosition() const
{
    return position;
}

void Transform::Translate(const glm::vec2& offset)
{
    position += offset;
}

void Transform::Translate(float x, float y)
{
    position.x += x;
    position.y += y;
}

void Transform::SetRotation(float degrees)
{
    rotation = degrees;
}

float Transform::GetRotation() const
{
    return rotation;
}

void Transform::Rotate(float degrees)
{
    rotation += degrees;
}

void Transform::SetScale(const glm::vec2& scale)
{
    this->scale = scale;
}

void Transform::SetScale(float x, float y)
{
    scale.x = x;
    scale.y = y;
}

const glm::vec2& Transform::GetScale() const
{
    return scale;
}
