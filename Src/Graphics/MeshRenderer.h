#pragma once

#include <memory>

#include "Camera3D.h"
#include "Shader.h"
#include "WorldObject.h"

/// Draws WorldObjects through the 3D pipeline.
///
/// For each object it builds the model matrix, uploads the camera's view and
/// projection matrices, sets the colour and lighting uniforms and submits one
/// indexed draw call.
class MeshRenderer
{
public:

    MeshRenderer(
        std::shared_ptr<Shader> shader,
        std::shared_ptr<Camera3D> camera);

    /// Uploads the light settings that stay constant for the whole scene.
    void Initialize();

    void Draw(const WorldObject& object);

    std::shared_ptr<Shader> GetShader() const;

private:

    std::shared_ptr<Shader> shader;

    std::shared_ptr<Camera3D> camera;
};
