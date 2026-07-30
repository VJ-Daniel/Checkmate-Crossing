#pragma once

#include <glm.hpp>

/// Holds a 3D object's position, Euler rotation in degrees and scale, and
/// builds the model matrix the world renderer uploads to the shader.
class Transform3D
{
public:

    Transform3D();

    Transform3D(
        const glm::vec3& position,
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    //---------------------------------------------------------
    // Position
    //---------------------------------------------------------

    void SetPosition(const glm::vec3& position);

    void SetPosition(float x, float y, float z);

    const glm::vec3& GetPosition() const;

    void Translate(const glm::vec3& offset);

    void Translate(float x, float y, float z);

    //---------------------------------------------------------
    // Rotation (degrees, applied in Y -> X -> Z order)
    //---------------------------------------------------------

    void SetRotation(const glm::vec3& degrees);

    void SetRotation(float x, float y, float z);

    const glm::vec3& GetRotation() const;

    void Rotate(const glm::vec3& degrees);

    //---------------------------------------------------------
    // Scale
    //---------------------------------------------------------

    void SetScale(const glm::vec3& scale);

    void SetScale(float x, float y, float z);

    const glm::vec3& GetScale() const;

    //---------------------------------------------------------
    // Matrix
    //---------------------------------------------------------

    /// Local space -> world space: translation * rotation * scale.
    glm::mat4 GetModelMatrix() const;

private:

    glm::vec3 position;

    glm::vec3 rotation;

    glm::vec3 scale;
};
