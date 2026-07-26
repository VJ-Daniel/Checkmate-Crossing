#pragma once

/* Enemy public API and state data for pursuit-based melee combat. */

#include <memory>

#include "AnimationClip.h"
#include "GameObject.h"

class Player;

enum class EnemyState
{
    Idle,
    Walking,
    Attacking,
    Hurt,
    Dead
};

/// Autonomous melee opponent that pursues a target Player.
class Enemy : public GameObject
{
public:
    explicit Enemy(const std::shared_ptr<Player>& target);

    void Initialize() override;
    void Update(float deltaTime) override;

    /// Restores health and places the enemy at the selected screen edge.
    void Spawn(bool fromLeft);
    void ReceiveDamage();
    bool IsDead() const;
    /// Becomes true three seconds after entering the Dead state.
    bool ShouldDisappear() const;

private:
    void LoadAnimations();
    void ChangeState(EnemyState newState);
    /// Chooses between walking toward the player and attacking in range.
    void UpdateAI(float deltaTime);
    void UpdateAnimation();
    /// Tests body overlap against the player's active impact frame.
    void CheckPlayerAttack();

    std::weak_ptr<Player> target;
    EnemyState state = EnemyState::Idle;
    int health = 3;
    float speed = 115.0f;
    float attackRange = 48.0f;
    bool attackDamageApplied = false;
    float deadElapsedTime = 0.0f;

    std::shared_ptr<AnimationClip> idleAnimation;
    std::shared_ptr<AnimationClip> walkAnimation;
    std::shared_ptr<AnimationClip> attackAnimation;
    std::shared_ptr<AnimationClip> hurtAnimation;
    std::shared_ptr<AnimationClip> deadAnimation;
};
