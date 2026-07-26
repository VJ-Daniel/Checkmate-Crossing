/*
    ============================================================
    Gangster Survival - Game Object
    Author: Leonardo Moura | Date: 2026

    Defines the common entity composition used by the scene:
    Transform, Sprite, Animator and active state.
    ============================================================
*/

#include "GameObject.h"

GameObject::GameObject()
    : active(true)
{
    animator.SetSprite(&sprite);
}

void GameObject::Initialize()
{
}

void GameObject::Update(float deltaTime)
{
    if (!active)
        return;

    animator.Update(deltaTime);
}

void GameObject::Render()
{
}

Transform& GameObject::GetTransform()
{
    return transform;
}

const Transform& GameObject::GetTransform() const
{
    return transform;
}

Sprite& GameObject::GetSprite()
{
    return sprite;
}

const Sprite& GameObject::GetSprite() const
{
    return sprite;
}

Animator& GameObject::GetAnimator()
{
    return animator;
}

const Animator& GameObject::GetAnimator() const
{
    return animator;
}

void GameObject::SetActive(bool active)
{
    this->active = active;
}

bool GameObject::IsActive() const
{
    return active;
}
