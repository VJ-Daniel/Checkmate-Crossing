#pragma once

#include <memory>

#include "Transform.h"
#include "Sprite.h"
#include "Animator.h"

/// Base scene entity combining transform, sprite, animator and active state.
class GameObject
{
public:

    GameObject();

    virtual ~GameObject() = default;

    virtual void Initialize();

    /// Default update advances the object's animator.
    virtual void Update(float deltaTime);

    virtual void Render();

    Transform& GetTransform();

    const Transform& GetTransform() const;

    Sprite& GetSprite();

    const Sprite& GetSprite() const;

    Animator& GetAnimator();

    const Animator& GetAnimator() const;

    void SetActive(bool active);

    bool IsActive() const;

protected:

    Transform transform;

    Sprite sprite;

    Animator animator;

    bool active;
};
