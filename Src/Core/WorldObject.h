#pragma once

#include <memory>

#include <glm.hpp>

#include "Mesh.h"
#include "Transform3D.h"

/// Base entity for anything that lives on the 3D battlefield: a transform,
/// the mesh it is drawn with, a flat colour and its active state.
///
/// Lanes, the pawn, and later obstacles, checkpoints, chess pieces and the
/// king's cage all derive from this, so the renderer only has to know about
/// one type.
class WorldObject
{
public:

    WorldObject();

    WorldObject(
        std::shared_ptr<Mesh> mesh,
        const glm::vec4& color);

    virtual ~WorldObject() = default;

    /// Called once after construction, before the first frame.
    virtual void Initialize();

    /// Called every frame with frame-independent elapsed time.
    /// The base object is static and does nothing.
    virtual void Update(float deltaTime);

    Transform3D& GetTransform();

    const Transform3D& GetTransform() const;

    /// The matrix this object is drawn with.
    ///
    /// For a plain object that is just its own transform. SceneNode overrides
    /// it to return the transform composed with its parents', which is what
    /// lets the renderer draw a flat list and a hierarchy through exactly the
    /// same path.
    virtual glm::mat4 GetWorldMatrix() const;

    void SetMesh(std::shared_ptr<Mesh> mesh);

    const std::shared_ptr<Mesh>& GetMesh() const;

    void SetColor(const glm::vec4& color);

    const glm::vec4& GetColor() const;

    /// Visible objects are drawn; invisible ones are skipped by the renderer.
    void SetVisible(bool visible);

    bool IsVisible() const;

    /// Active objects receive Update; inactive ones are frozen.
    void SetActive(bool active);

    bool IsActive() const;

protected:

    Transform3D transform;

    std::shared_ptr<Mesh> mesh;

    glm::vec4 color;

    bool visible;

    bool active;
};
