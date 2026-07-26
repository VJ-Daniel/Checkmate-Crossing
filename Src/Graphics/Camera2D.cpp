/*
    ============================================================
    Gangster Survival - 2D Camera
    Author: Leonardo Moura | Date: 2026

    Builds orthographic projection and view matrices so game-world
    units behave like screen pixels while supporting movement/zoom.
    ============================================================
*/

#include "Camera2D.h"

Camera2D::Camera2D()
    : screenWidth(800.0f),
    screenHeight(600.0f),
    position(0.0f, 0.0f),
    zoom(1.0f)
{
    UpdateProjection();
    UpdateView();
}

Camera2D::Camera2D(
    float screenWidth,
    float screenHeight)
    : screenWidth(screenWidth),
    screenHeight(screenHeight),
    position(0.0f, 0.0f),
    zoom(1.0f)
{
    UpdateProjection();
    UpdateView();
}

void Camera2D::SetViewport(
    float screenWidth,
    float screenHeight)
{
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;

    UpdateProjection();
}

void Camera2D::SetPosition(
    const glm::vec2& position)
{
    this->position = position;

    UpdateView();
}

void Camera2D::Move(
    const glm::vec2& offset)
{
    position += offset;

    UpdateView();
}

const glm::vec2& Camera2D::GetPosition() const
{
    return position;
}

void Camera2D::SetZoom(
    float zoom)
{
    if (zoom < 0.01f)
    {
        zoom = 0.01f;
    }

    this->zoom = zoom;

    UpdateProjection();
}

float Camera2D::GetZoom() const
{
    return zoom;
}

const glm::mat4& Camera2D::GetProjectionMatrix() const
{
    return projection;
}

const glm::mat4& Camera2D::GetViewMatrix() const
{
    return view;
}

void Camera2D::Update()
{
    UpdateProjection();
    UpdateView();
}

void Camera2D::UpdateProjection()
{
    float halfWidth = (screenWidth * 0.5f) / zoom;
    float halfHeight = (screenHeight * 0.5f) / zoom;

    projection = glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        -1.0f,
        1.0f);
}

void Camera2D::UpdateView()
{
    view = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            -position.x,
            -position.y,
            0.0f));
}
