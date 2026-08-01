#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

/// How the camera flattens the 3D world onto the screen.
enum class ProjectionMode
{
    /// Parallel projection: distant objects keep their size and cube edges
    /// stay parallel. This is what the reference game uses and what makes
    /// the battlefield read like a board seen from above.
    Orthographic,

    /// Classic 3D projection with vanishing points, described in GDD 5.
    /// Kept available so the same scene can be shown either way.
    Perspective
};

/// Axis-aligned world-space footprint made where the four corners of an
/// orthographic viewport look through a horizontal plane. The camera clamp
/// uses this instead of pretending its target is the edge of what is visible.
struct CameraGroundBounds
{
    float minX = 0.0f;
    float maxX = 0.0f;
    float minZ = 0.0f;
    float maxZ = 0.0f;
};

/// Fixed-angle world camera for the battlefield.
///
/// The camera is described by the point it looks at (its target) plus an
/// orbit: a pitch angle below the horizon, a yaw around the world, and a
/// distance. It never rolls and the player never rotates it, which matches
/// both the reference game and GDD section 5.
///
/// Turning this into a follow camera means nothing more than calling
/// SetTarget every frame with the pawn's position.
class Camera3D
{
public:

    Camera3D();

    Camera3D(
        float screenWidth,
        float screenHeight);

    /// Keeps the projection correct when the window is resized.
    void SetViewport(
        float screenWidth,
        float screenHeight);

    //---------------------------------------------------------
    // Framing
    //---------------------------------------------------------

    /// The point the camera orbits around and looks at.
    void SetTarget(const glm::vec3& target);

    const glm::vec3& GetTarget() const;

    /// Angle below the horizon, in degrees. Clamped away from straight
    /// down, where the "up" direction would become ambiguous.
    void SetPitch(float degrees);

    float GetPitch() const;

    void SetYaw(float degrees);

    float GetYaw() const;

    /// How far back the camera sits along its orbit. In orthographic mode
    /// this does not change how large objects appear, only what stays
    /// inside the near and far planes.
    void SetDistance(float distance);

    float GetDistance() const;

    //---------------------------------------------------------
    // Projection
    //---------------------------------------------------------

    /// Vertical slice of the world the camera shows, in world units.
    /// This is the orthographic camera's zoom control.
    void SetOrthographicHeight(float height);

    float GetOrthographicHeight() const;

    /// Vertical field of view in degrees, used in perspective mode only.
    void SetFieldOfView(float degrees);

    float GetFieldOfView() const;

    void SetClipPlanes(float nearPlane, float farPlane);

    void SetProjectionMode(ProjectionMode mode);

    ProjectionMode GetProjectionMode() const;

    //---------------------------------------------------------
    // Matrices
    //---------------------------------------------------------

    /// World-space position derived from target, pitch, yaw and distance.
    glm::vec3 GetPosition() const;

    const glm::mat4& GetViewMatrix() const;

    const glm::mat4& GetProjectionMatrix() const;

    /// Intersects the four orthographic corner rays with y = groundHeight
    /// and returns their X/Z bounds. This accounts for viewport aspect,
    /// orthographic height, pitch, yaw and target height; camera distance
    /// cancels because orthographic rays are parallel.
    CameraGroundBounds GetOrthographicGroundBounds(
        float groundHeight) const;

    /// Recomputes both matrices. Called automatically by every setter.
    void Update();

private:

    void UpdateProjection();

    void UpdateView();

private:

    float screenWidth;

    float screenHeight;

    glm::vec3 target;

    float pitch;

    float yaw;

    float distance;

    float orthographicHeight;

    float fieldOfView;

    float nearPlane;

    float farPlane;

    ProjectionMode projectionMode;

    glm::mat4 projection;

    glm::mat4 view;
};
