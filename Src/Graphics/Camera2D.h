#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

/// Orthographic camera that maps world units directly to screen-like pixels.
class Camera2D
{
public:

    Camera2D();

    Camera2D(
        float screenWidth,
        float screenHeight);

    void SetViewport(
        float screenWidth,
        float screenHeight);

    void SetPosition(
        const glm::vec2& position);

    void Move(
        const glm::vec2& offset);

    const glm::vec2& GetPosition() const;

    void SetZoom(
        float zoom);

    float GetZoom() const;

    const glm::mat4& GetProjectionMatrix() const;

    const glm::mat4& GetViewMatrix() const;

    void Update();

private:

    void UpdateProjection();

    void UpdateView();

private:

    float screenWidth;

    float screenHeight;

    glm::vec2 position;

    float zoom;

    glm::mat4 projection;

    glm::mat4 view;
};
