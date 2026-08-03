/*
    ============================================================
    Checkmate Crossing - Mesh Renderer

    Turns a WorldObject into a draw call: model / view / projection
    matrices, flat colour and directional lighting uniforms, then one
    indexed submission of the object's mesh.
    ============================================================
*/

#include "MeshRenderer.h"

#include "GameConfig.h"

MeshRenderer::MeshRenderer(
    std::shared_ptr<Shader> shader,
    std::shared_ptr<Camera3D> camera)
    : shader(std::move(shader)),
    camera(std::move(camera))
{
}

void MeshRenderer::Initialize()
{
    if (!shader)
        return;

    shader->Use();

    // One directional light for the whole battlefield, so it only has to be
    // uploaded once instead of every frame.
    shader->SetVector3(
        "lightDirection",
        GameConfig::LightDirection);

    shader->SetFloat(
        "ambientStrength",
        GameConfig::AmbientStrength);
}

void MeshRenderer::Draw(const WorldObject& object)
{
    if (!shader || !camera)
        return;

    if (!object.IsVisible())
        return;

    const auto& mesh = object.GetMesh();

    if (!mesh || !mesh->IsValid())
        return;

    shader->Use();

    shader->SetMatrix4(
        "projection",
        camera->GetProjectionMatrix());

    shader->SetMatrix4(
        "view",
        camera->GetViewMatrix());

    // Through GetWorldMatrix, not the transform directly: a rig part's
    // transform is relative to its parent, and only the object itself knows
    // how to resolve that.
    shader->SetMatrix4(
        "model",
        object.GetWorldMatrix());

    shader->SetVector4(
        "objectColor",
        object.GetColor());

    mesh->Draw();
}

std::shared_ptr<Shader> MeshRenderer::GetShader() const
{
    return shader;
}
