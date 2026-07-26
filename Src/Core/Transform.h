#pragma once

#include <glm.hpp>

/// Holds a 2D object's position, rotation in degrees and rendered size.
class Transform
{
public:

    Transform();

    Transform(
        const glm::vec2& position,
        float rotation = 0.0f,
        const glm::vec2& scale = glm::vec2(1.0f));

    //---------------------------------------------------------
    // Position
    //---------------------------------------------------------

    void SetPosition(const glm::vec2& position);

    void SetPosition(float x, float y);

    const glm::vec2& GetPosition() const;

    void Translate(const glm::vec2& offset);

    void Translate(float x, float y);

    //---------------------------------------------------------
    // Rotation
    //---------------------------------------------------------

    void SetRotation(float degrees);

    float GetRotation() const;

    void Rotate(float degrees);

    //---------------------------------------------------------
    // Scale
    //---------------------------------------------------------

    void SetScale(const glm::vec2& scale);

    void SetScale(float x, float y);

    const glm::vec2& GetScale() const;

private:

    glm::vec2 position;

    float rotation;

    glm::vec2 scale;
};
