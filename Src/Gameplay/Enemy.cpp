/*
    ============================================================
    Gangster Survival - Enemy Module

    Author: Leonardo Moura
    Date: 2026

    Description:
    Implements enemy sprite animations, pursuit AI, melee range,
    hit detection, health, damage, death and timed removal.
    ============================================================
*/

#include "Enemy.h"

#include <cmath>

#include "AudioPlayer.h"
#include "Player.h"
#include "ResourceManager.h"

namespace
{
    std::shared_ptr<AnimationClip> MakeAnimation(
        const std::shared_ptr<SpriteSheet>& sheet,
        int frameCount, float frameDuration, bool loop)
    {
        auto animation = std::make_shared<AnimationClip>(loop);
        animation->SetSpriteSheet(sheet);

        for (int frame = 0; frame < frameCount; ++frame)
            animation->AddFrame(AnimationFrame(frame, 0, frameDuration));

        return animation;
    }
}

Enemy::Enemy(const std::shared_ptr<Player>& target)
    : target(target)
{
}

void Enemy::Initialize()
{
    LoadAnimations();
    GetTransform().SetScale(128.0f, 128.0f);
    GetAnimator().Play(idleAnimation);
}

void Enemy::LoadAnimations()
{
    const char* folder = "Src/Resources/Characters/Enemy_01/";

    ResourceManager::LoadTexture("Enemy_Idle", std::string(folder) + "Idle.png");
    ResourceManager::LoadTexture("Enemy_Walk", std::string(folder) + "Walk.png");
    ResourceManager::LoadTexture("Enemy_Attack", std::string(folder) + "Attack.png");
    ResourceManager::LoadTexture("Enemy_Hurt", std::string(folder) + "Hurt.png");
    ResourceManager::LoadTexture("Enemy_Dead", std::string(folder) + "Dead.png");

    idleAnimation = MakeAnimation(ResourceManager::CreateSpriteSheet(
        "EnemyIdleSheet", "Enemy_Idle", 7, 1), 7, 0.10f, true);
    walkAnimation = MakeAnimation(ResourceManager::CreateSpriteSheet(
        "EnemyWalkSheet", "Enemy_Walk", 10, 1), 10, 0.08f, true);
    attackAnimation = MakeAnimation(ResourceManager::CreateSpriteSheet(
        "EnemyAttackSheet", "Enemy_Attack", 5, 1), 5, 0.10f, false);
    hurtAnimation = MakeAnimation(ResourceManager::CreateSpriteSheet(
        "EnemyHurtSheet", "Enemy_Hurt", 4, 1), 4, 0.10f, false);
    deadAnimation = MakeAnimation(ResourceManager::CreateSpriteSheet(
        "EnemyDeadSheet", "Enemy_Dead", 5, 1), 5, 0.15f, false);
}

void Enemy::Spawn(bool fromLeft)
{
    health = 3;
    deadElapsedTime = 0.0f;
    GetTransform().SetPosition(fromLeft ? -620.0f : 492.0f, -300.0f);
    GetSprite().SetFlipX(!fromLeft);
    ChangeState(EnemyState::Walking);
}

void Enemy::Update(float deltaTime)
{
    if (state == EnemyState::Dead)
        deadElapsedTime += deltaTime;

    CheckPlayerAttack();
    UpdateAI(deltaTime);
    GameObject::Update(deltaTime);
    UpdateAnimation();
}

void Enemy::CheckPlayerAttack()
{
    auto player = target.lock();
    if (!player || state == EnemyState::Dead || state == EnemyState::Hurt)
        return;

    const glm::vec2 playerPosition = player->GetTransform().GetPosition();
    const glm::vec2 enemyPosition = GetTransform().GetPosition();
    const float distance = std::abs(
        (playerPosition.x + 64.0f) - (enemyPosition.x + 64.0f));
    const bool verticalOverlap =
        playerPosition.y + 108.0f >= enemyPosition.y + 16.0f &&
        playerPosition.y + 16.0f <= enemyPosition.y + 108.0f;

    if (distance <= 55.0f && verticalOverlap &&
        player->CanDealAttackDamage())
    {
        player->ConfirmAttackDamage();
        ReceiveDamage();
    }
}

void Enemy::UpdateAI(float deltaTime)
{
    auto player = target.lock();
    if (!player || player->IsDead() || state == EnemyState::Dead ||
        state == EnemyState::Hurt)
        return;

    const glm::vec2 playerPosition = player->GetTransform().GetPosition();
    const glm::vec2 enemyPosition = GetTransform().GetPosition();
    const float difference = playerPosition.x - enemyPosition.x;
    const float distance = std::abs(difference);
    const bool verticalOverlap =
        playerPosition.y + 108.0f >= enemyPosition.y + 16.0f &&
        playerPosition.y + 16.0f <= enemyPosition.y + 108.0f;

    if (state == EnemyState::Attacking)
    {
        if (!attackDamageApplied && GetAnimator().GetCurrentFrame() >= 2 &&
            distance <= attackRange && verticalOverlap)
        {
            attackDamageApplied = true;
            player->ReceiveDamage();
        }
        return;
    }

    GetSprite().SetFlipX(difference < 0.0f);

    if (distance <= attackRange && verticalOverlap)
    {
        ChangeState(EnemyState::Attacking);
        return;
    }

    ChangeState(EnemyState::Walking);
    GetTransform().Translate((difference < 0.0f ? -1.0f : 1.0f) *
        speed * deltaTime, 0.0f);
}

void Enemy::UpdateAnimation()
{
    if (state == EnemyState::Attacking && GetAnimator().IsFinished())
        ChangeState(EnemyState::Idle);
    else if (state == EnemyState::Hurt && GetAnimator().IsFinished())
        ChangeState(EnemyState::Idle);
}

void Enemy::ChangeState(EnemyState newState)
{
    if (state == newState)
        return;

    state = newState;

    switch (state)
    {
    case EnemyState::Idle:
        GetAnimator().Play(idleAnimation);
        break;
    case EnemyState::Walking:
        GetAnimator().Play(walkAnimation);
        break;
    case EnemyState::Attacking:
        attackDamageApplied = false;
        GetAnimator().Play(attackAnimation);
        break;
    case EnemyState::Hurt:
        GetAnimator().Play(hurtAnimation);
        break;
    case EnemyState::Dead:
        GetAnimator().Play(deadAnimation);
        break;
    }
}

void Enemy::ReceiveDamage()
{
    if (state == EnemyState::Dead)
        return;

    --health;

    const bool fatal = health <= 0;
    ChangeState(fatal ? EnemyState::Dead : EnemyState::Hurt);
    AudioPlayer::PlayPunch(fatal);
}

bool Enemy::IsDead() const
{
    return state == EnemyState::Dead;
}

bool Enemy::ShouldDisappear() const
{
    return state == EnemyState::Dead && deadElapsedTime >= 3.0f;
}
