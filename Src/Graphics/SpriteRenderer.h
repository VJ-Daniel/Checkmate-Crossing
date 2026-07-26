#pragma once

#include <memory>

#include <glm.hpp>
#include <glew.h>

#include "Shader.h"
#include "Camera2D.h"
#include "Transform.h"
#include "Sprite.h"

/// Batched-independent quad renderer for textured 2D sprite-sheet frames.
class SpriteRenderer
{
public:

    SpriteRenderer(
        std::shared_ptr<Shader> shader,
        std::shared_ptr<Camera2D> camera);

    ~SpriteRenderer();

    void Initialize();

    /// Builds the model matrix, selects UVs and submits one indexed quad.
    void Draw(
        const Transform& transform,
        const Sprite& sprite);

    void Destroy();

private:

    void BuildQuad();

    std::shared_ptr<Shader> shader;

    std::shared_ptr<Camera2D> camera;

    GLuint vao;

    GLuint vbo;

    GLuint ebo;
};
