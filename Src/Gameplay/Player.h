#pragma once

#include <memory>

#include <glm.hpp>

#include "GameObject.h"
#include "AnimationClip.h"
#include "Input.h"
#include "ResourceManager.h"

enum class PlayerState
{
    Idle,
    Walking,
    Running,
    Jumping,
    Falling,
    Attack1,
    Attack2,
    Attack3,
    Hurt,
    Dead
};

/// Controllable animated character with movement, combat and health states.
class Player : public GameObject
{
public:

    Player();

    ~Player() override;

    /// Loads sprite sheets, builds animation clips and sets the spawn pose.
    void Initialize() override;

    /// Processes input, physics-like movement and animation transitions.
    void Update(float deltaTime) override;

    float GetSpeed() const;

    void SetSpeed(float speed);

    /// Returns true only during the impact frame of the current attack.
    bool CanDealAttackDamage() const;

    /// Marks the current attack as consumed so it cannot hit twice.
    void ConfirmAttackDamage();

    /// Removes one health point and selects Hurt or Dead state.
    void ReceiveDamage();

    bool IsDead() const;

    int GetHealth() const;

private:

    /// Maps keyboard and gamepad controls into player state changes.
    void HandleInput();

    void LoadSpriteSheets();

    void LoadAnimations();

    /// Applies horizontal velocity, jump gravity and screen boundaries.
    void UpdateMovement(float deltaTime);

    void UpdateAnimation();

    /// Switches the active animation and related movement audio.
    void ChangeState(PlayerState newState);

    PlayerState GetState() const;

    bool IsMoving() const;

    void Stop();

    void Kill();

    void TakeDamage();

private:

    PlayerState state = PlayerState::Idle;

    float speed = 250.0f;

    float verticalVelocity = 0.0f;

    float groundY = -300.0f;

    bool jumpWasPressed = false;

    // Requires the attack keys to be released before another blow starts.
    bool attackReady = true;

    bool attackDamageApplied = false;

    int health = 5;

    bool moving = false;

    glm::vec2 movement = glm::vec2(0.0f);

private:

    std::shared_ptr<AnimationClip> idleAnimation;
    std::shared_ptr<AnimationClip> walkAnimation;
    std::shared_ptr<AnimationClip> runAnimation;
    std::shared_ptr<AnimationClip> jumpAnimation;
    std::shared_ptr<AnimationClip> attack1Animation;
    std::shared_ptr<AnimationClip> attack2Animation;
    std::shared_ptr<AnimationClip> attack3Animation;
    std::shared_ptr<AnimationClip> hurtAnimation;
    std::shared_ptr<AnimationClip> deadAnimation;

private:

    std::shared_ptr<SpriteSheet> idleSheet;
    std::shared_ptr<SpriteSheet> walkSheet;
    std::shared_ptr<SpriteSheet> runSheet;
    std::shared_ptr<SpriteSheet> jumpSheet;
    std::shared_ptr<SpriteSheet> attack1Sheet;
    std::shared_ptr<SpriteSheet> attack2Sheet;
    std::shared_ptr<SpriteSheet> attack3Sheet;
    std::shared_ptr<SpriteSheet> hurtSheet;
    std::shared_ptr<SpriteSheet> deadSheet;
};
