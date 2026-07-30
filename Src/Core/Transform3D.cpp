/*
    ============================================================
    Checkmate Crossing - 3D Transform

    Stores and modifies 3D position, Euler rotation and scale, and
    combines them into the model matrix used by the world renderer.
    ============================================================
*/

#include "Transform3D.h"

#include <gtc/matrix_transform.hpp>

Transform3D::Transform3D()
    : position(0.0f),
    rotation(0.0f),
    scale(1.0f)
{
}

Transform3D::Transform3D(
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
    : position(position),
    rotation(rotation),
    scale(scale)
{
}

void Transform3D::SetPosition(const glm::vec3& position)
{
    this->position = position;
}

void Transform3D::SetPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
}

const glm::vec3& Transform3D::GetPosition() const
{
    return position;
}

void Transform3D::Translate(const glm::vec3& offset)
{
    position += offset;
}

void Transform3D::Translate(float x, float y, float z)
{
    position.x += x;
    position.y += y;
    position.z += z;
}

void Transform3D::SetRotation(const glm::vec3& degrees)
{
    rotation = degrees;
}

void Transform3D::SetRotation(float x, float y, float z)
{
    rotation.x = x;
    rotation.y = y;
    rotation.z = z;
}

const glm::vec3& Transform3D::GetRotation() const
{
    return rotation;
}

void Transform3D::Rotate(const glm::vec3& degrees)
{
    rotation += degrees;
}

void Transform3D::SetScale(const glm::vec3& scale)
{
    this->scale = scale;
}

void Transform3D::SetScale(float x, float y, float z)
{
    scale.x = x;
    scale.y = y;
    scale.z = z;
}

const glm::vec3& Transform3D::GetScale() const
{
    return scale;
}

glm::mat4 Transform3D::GetModelMatrix() const
{
    glm::mat4 model(1.0f);

    model = glm::translate(model, position);

    // Yaw first, then pitch, then roll. Yaw is the axis chess pieces and
    // obstacles will actually use, so it is applied closest to the object.
    model = glm::rotate(
        model,
        glm::radians(rotation.y),
        glm::vec3(0.0f, 1.0f, 0.0f));

    model = glm::rotate(
        model,
        glm::radians(rotation.x),
        glm::vec3(1.0f, 0.0f, 0.0f));

    model = glm::rotate(
        model,
        glm::radians(rotation.z),
        glm::vec3(0.0f, 0.0f, 1.0f));

    model = glm::scale(model, scale);

    return model;
}
