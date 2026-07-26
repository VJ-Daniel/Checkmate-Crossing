/*
    ============================================================
    Gangster Survival - Sprite Renderer
    Author: Leonardo Moura | Date: 2026

    Creates a reusable indexed quad, calculates model transforms and
    UV ranges, binds textures and submits 2D OpenGL draw calls.
    ============================================================
*/

#include "SpriteRenderer.h"

#include <gtc/matrix_transform.hpp>

SpriteRenderer::SpriteRenderer(
    std::shared_ptr<Shader> shader,
    std::shared_ptr<Camera2D> camera)
    : shader(shader),
    camera(camera),
    vao(0),
    vbo(0),
    ebo(0)
{
}

SpriteRenderer::~SpriteRenderer()
{
    Destroy();
}

void SpriteRenderer::Initialize()
{
    BuildQuad();
}

void SpriteRenderer::BuildQuad()
{
    float vertices[] =
    {
        // Position          // UV
         0.0f, 0.0f, 0.0f,    0.0f, 1.0f,
         1.0f, 0.0f, 0.0f,    1.0f, 1.0f,
         1.0f, 1.0f, 0.0f,    1.0f, 0.0f,
         0.0f, 1.0f, 0.0f,    0.0f, 0.0f
    };

    unsigned int indices[] =
    {
        0,1,2,
        2,3,0
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW);

    glBindBuffer(
        GL_ELEMENT_ARRAY_BUFFER,
        ebo);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)0);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void SpriteRenderer::Draw(
    const Transform& transform,
    const Sprite& sprite)
{
    if (!sprite.IsVisible())
        return;

    auto sheet = sprite.GetSpriteSheet();

    if (!sheet)
        return;

    auto texture = sheet->GetTexture();

    if (!texture)
        return;

    shader->Use();

    shader->SetMatrix4(
        "projection",
        camera->GetProjectionMatrix());

    shader->SetMatrix4(
        "view",
        camera->GetViewMatrix());

    glm::mat4 model(1.0f);

    model = glm::translate(
        model,
        glm::vec3(
            transform.GetPosition(),
            0.0f));

    model = glm::translate(
        model,
        glm::vec3(
            transform.GetScale().x * 0.5f,
            transform.GetScale().y * 0.5f,
            0.0f));

    model = glm::rotate(
        model,
        glm::radians(
            transform.GetRotation()),
        glm::vec3(
            0.0f,
            0.0f,
            1.0f));

    model = glm::translate(
        model,
        glm::vec3(
            -transform.GetScale().x * 0.5f,
            -transform.GetScale().y * 0.5f,
            0.0f));

    model = glm::scale(
        model,
        glm::vec3(
            transform.GetScale(),
            1.0f));

    shader->SetMatrix4(
        "model",
        model);

    shader->SetVector4(
        "spriteColor",
        sprite.GetTint());

    glm::vec2 uvMin;
    glm::vec2 uvMax;

    sheet->GetUV(
        sprite.GetColumn(),
        sprite.GetRow(),
        uvMin,
        uvMax);

    if (sprite.GetFlipX())
        std::swap(uvMin.x, uvMax.x);

    if (sprite.GetFlipY())
        std::swap(uvMin.y, uvMax.y);

    shader->SetVector2("uvMin", uvMin);
    shader->SetVector2("uvMax", uvMax);

    texture->Bind();

    glBindVertexArray(vao);

    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        nullptr);

    glBindVertexArray(0);

    texture->Unbind();
}

void SpriteRenderer::Destroy()
{
    if (ebo != 0)
    {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }

    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}
