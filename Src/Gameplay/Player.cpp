/*
    ============================================================
    Gangster Survival - Player Module

    Author: Leonardo Moura
    Date: 2026

    Description:
    Implements player input, movement, jumping, animation states,
    melee attacks, health, damage, death and movement audio.
    ============================================================
*/

#include "Player.h"

#include "AudioPlayer.h"

namespace
{
    constexpr const wchar_t* WalkingSound =
        L"Src/Resources/Sounds/WalkingOnGrass.wav";
    constexpr const wchar_t* RunningSound =
        L"Src/Resources/Sounds/RunningOnGrass.wav";

    bool IsMovingLeft()
    {
        return Input::IsKeyPressed(Key::A) ||
            Input::IsKeyPressed(Key::Left) ||
            Input::IsGamepadButtonPressed(GamepadButton::DPadLeft) ||
            Input::GetGamepadLeftStick().x < 0.0f;
    }

    bool IsMovingRight()
    {
        return Input::IsKeyPressed(Key::D) ||
            Input::IsKeyPressed(Key::Right) ||
            Input::IsGamepadButtonPressed(GamepadButton::DPadRight) ||
            Input::GetGamepadLeftStick().x > 0.0f;
    }

    bool IsRunningPressed()
    {
        return Input::IsKeyPressed(Key::LeftShift) ||
            Input::IsGamepadButtonPressed(GamepadButton::LeftBumper);
    }
}

Player::Player()
    : state(PlayerState::Idle),
    speed(250.0f),
    moving(false),
    movement(0.0f, 0.0f)
{
}

Player::~Player()
{
    AudioPlayer::Stop();
}

void Player::Initialize()
{
    LoadSpriteSheets();

    LoadAnimations();

    state = PlayerState::Idle;
    GetAnimator().Play(idleAnimation);

    GetTransform().SetPosition(-64.0f, groundY);
    GetTransform().SetScale(128.0f, 128.0f);
}

void Player::Update(float deltaTime)
{
    HandleInput();
    UpdateMovement(deltaTime);
    GameObject::Update(deltaTime);
    UpdateAnimation();
}

void Player::LoadSpriteSheets()
{
    //
    // TEXTURES
    //

    ResourceManager::LoadTexture(
        "Gangster_Idle",
        "Src/Resources/Characters/Gangster/Idle.png");

    ResourceManager::LoadTexture(
        "Gangster_Walk",
        "Src/Resources/Characters/Gangster/Walk.png");

    ResourceManager::LoadTexture(
        "Gangster_Run",
        "Src/Resources/Characters/Gangster/Run.png");

    ResourceManager::LoadTexture(
        "Gangster_Jump",
        "Src/Resources/Characters/Gangster/Jump.png");

    ResourceManager::LoadTexture(
        "Gangster_Attack1",
        "Src/Resources/Characters/Gangster/Attack_1.png");

    ResourceManager::LoadTexture(
        "Gangster_Attack2",
        "Src/Resources/Characters/Gangster/Attack_2.png");

    ResourceManager::LoadTexture(
        "Gangster_Attack3",
        "Src/Resources/Characters/Gangster/Attack_3.png");

    ResourceManager::LoadTexture(
        "Gangster_Hurt",
        "Src/Resources/Characters/Gangster/Hurt.png");

    ResourceManager::LoadTexture(
        "Gangster_Dead",
        "Src/Resources/Characters/Gangster/Dead.png");

    //
    // SPRITE SHEETS
    //

    idleSheet =
        ResourceManager::CreateSpriteSheet(
            "IdleSheet",
            "Gangster_Idle",
            7,
            1);

    walkSheet =
        ResourceManager::CreateSpriteSheet(
            "WalkSheet",
            "Gangster_Walk",
            10,
            1);

    runSheet =
        ResourceManager::CreateSpriteSheet(
            "RunSheet",
            "Gangster_Run",
            10,
            1);

    jumpSheet =
        ResourceManager::CreateSpriteSheet(
            "JumpSheet",
            "Gangster_Jump",
            10,
            1);

    attack1Sheet =
        ResourceManager::CreateSpriteSheet(
            "Attack1Sheet",
            "Gangster_Attack1",
            6,
            1);

    attack2Sheet =
        ResourceManager::CreateSpriteSheet(
            "Attack2Sheet",
            "Gangster_Attack2",
            4,
            1);

    attack3Sheet =
        ResourceManager::CreateSpriteSheet(
            "Attack3Sheet",
            "Gangster_Attack3",
            6,
            1);

    hurtSheet =
        ResourceManager::CreateSpriteSheet(
            "HurtSheet",
            "Gangster_Hurt",
            4,
            1);

    deadSheet =
        ResourceManager::CreateSpriteSheet(
            "DeadSheet",
            "Gangster_Dead",
            5,
            1);
}

void Player::LoadAnimations()
{
    //
    // IDLE
    //

    idleAnimation = std::make_shared<AnimationClip>(true);
    idleAnimation->SetSpriteSheet(idleSheet);

    for (int i = 0; i < 7; ++i)
    {
        idleAnimation->AddFrame(
            AnimationFrame(i, 0, 0.10f));
    }

    //
    // WALK
    //

    walkAnimation = std::make_shared<AnimationClip>(true);
    walkAnimation->SetSpriteSheet(walkSheet);

    for (int i = 0; i < 10; ++i)
    {
        walkAnimation->AddFrame(
            AnimationFrame(i, 0, 0.08f));
    }

    //
    // RUN
    //

    runAnimation = std::make_shared<AnimationClip>(true);
    runAnimation->SetSpriteSheet(runSheet);

    for (int i = 0; i < 10; ++i)
    {
        runAnimation->AddFrame(
            AnimationFrame(i, 0, 0.06f));
    }

    //
    // JUMP
    //

    jumpAnimation = std::make_shared<AnimationClip>(false);
    jumpAnimation->SetSpriteSheet(jumpSheet);

    for (int i = 0; i < 10; ++i)
    {
        jumpAnimation->AddFrame(
            AnimationFrame(i, 0, 0.08f));
    }

    //
    // ATTACK 1
    //

    attack1Animation = std::make_shared<AnimationClip>(false);
    attack1Animation->SetSpriteSheet(attack1Sheet);

    for (int i = 0; i < 6; ++i)
    {
        attack1Animation->AddFrame(
            AnimationFrame(i, 0, 0.07f));
    }

    //
    // ATTACK 2
    //

    attack2Animation = std::make_shared<AnimationClip>(false);
    attack2Animation->SetSpriteSheet(attack2Sheet);

    for (int i = 0; i < 4; ++i)
    {
        attack2Animation->AddFrame(
            AnimationFrame(i, 0, 0.08f));
    }

    //
    // ATTACK 3
    //

    attack3Animation = std::make_shared<AnimationClip>(false);
    attack3Animation->SetSpriteSheet(attack3Sheet);

    for (int i = 0; i < 6; ++i)
    {
        attack3Animation->AddFrame(
            AnimationFrame(i, 0, 0.07f));
    }

    //
    // HURT
    //

    hurtAnimation = std::make_shared<AnimationClip>(false);
    hurtAnimation->SetSpriteSheet(hurtSheet);

    for (int i = 0; i < 4; ++i)
    {
        hurtAnimation->AddFrame(
            AnimationFrame(i, 0, 0.10f));
    }

    //
    // DEAD
    //

    deadAnimation = std::make_shared<AnimationClip>(false);
    deadAnimation->SetSpriteSheet(deadSheet);

    for (int i = 0; i < 5; ++i)
    {
        deadAnimation->AddFrame(
            AnimationFrame(i, 0, 0.15f));
    }
}

void Player::ChangeState(PlayerState newState)
{
    //
    // Do nothing if the state is already active.
    //

    if (state == newState)
    {
        return;
    }

    state = newState;

    Animator& animator = GetAnimator();

    switch (state)
    {
    case PlayerState::Idle:

        AudioPlayer::Stop();
        animator.Play(idleAnimation);

        break;

    case PlayerState::Walking:

        AudioPlayer::PlayLoop(WalkingSound);
        animator.Play(walkAnimation);

        break;

    case PlayerState::Running:

        AudioPlayer::PlayLoop(RunningSound);
        animator.Play(runAnimation);

        break;

    case PlayerState::Jumping:

        AudioPlayer::Stop();
        animator.Play(jumpAnimation);

        break;

    case PlayerState::Falling:

        //
        // Temporary:
        // Use Jump animation until a Falling animation exists.
        //

        AudioPlayer::Stop();
        animator.Play(jumpAnimation);

        break;

    case PlayerState::Attack1:

        AudioPlayer::Stop();
        attackDamageApplied = false;
        animator.Play(attack1Animation);

        break;

    case PlayerState::Attack2:

        AudioPlayer::Stop();
        attackDamageApplied = false;
        animator.Play(attack2Animation);

        break;

    case PlayerState::Attack3:

        AudioPlayer::Stop();
        attackDamageApplied = false;
        animator.Play(attack3Animation);

        break;

    case PlayerState::Hurt:

        AudioPlayer::Stop();
        animator.Play(hurtAnimation);

        break;

    case PlayerState::Dead:

        AudioPlayer::Stop();
        animator.Play(deadAnimation);

        break;

    default:

        break;
    }
}

void Player::HandleInput()
{
    const bool attack1Pressed = Input::IsKeyPressed(Key::J) ||
        Input::IsGamepadButtonPressed(GamepadButton::X);
    const bool attack2Pressed = Input::IsKeyPressed(Key::K) ||
        Input::IsGamepadButtonPressed(GamepadButton::Y);
    const bool attack3Pressed = Input::IsKeyPressed(Key::L) ||
        Input::IsGamepadButtonPressed(GamepadButton::B);

    if (!attack1Pressed && !attack2Pressed && !attack3Pressed)
        attackReady = true;

    //
    // Ignore input only for terminal/action animations.
    //

    switch (state)
    {
    case PlayerState::Attack1:
    case PlayerState::Attack2:
    case PlayerState::Attack3:
    case PlayerState::Hurt:
    case PlayerState::Dead:
        return;

    default:
        break;
    }

    const bool left = IsMovingLeft();
    const bool right = IsMovingRight();
    moving = left || right;

    //
    // Attack
    //

    const bool isGrounded =
        GetTransform().GetPosition().y <= groundY;

    if (attackReady && isGrounded && attack1Pressed)
    {
        attackReady = false;
        moving = false;
        ChangeState(PlayerState::Attack1);
        return;
    }

    if (attackReady && isGrounded && attack2Pressed)
    {
        attackReady = false;
        moving = false;
        ChangeState(PlayerState::Attack2);
        return;
    }

    if (attackReady && isGrounded && attack3Pressed)
    {
        attackReady = false;
        moving = false;
        ChangeState(PlayerState::Attack3);
        return;
    }

    //
    // Jump
    //

    const bool jumpPressed = Input::IsKeyPressed(Key::Space) ||
        Input::IsGamepadButtonPressed(GamepadButton::A);

    if (jumpPressed && !jumpWasPressed &&
        GetTransform().GetPosition().y <= groundY)
    {
        verticalVelocity = 620.0f;
        ChangeState(PlayerState::Jumping);
    }

    jumpWasPressed = jumpPressed;

    if (state == PlayerState::Jumping || state == PlayerState::Falling)
        return;

    //
    // Movement
    //

    if (!moving)
    {
        ChangeState(PlayerState::Idle);
        return;
    }

    if (IsRunningPressed())
    {
        ChangeState(PlayerState::Running);
    }
    else
    {
        ChangeState(PlayerState::Walking);
    }
}

void Player::UpdateMovement(float deltaTime)
{
    movement = glm::vec2(0.0f);

    const bool left = IsMovingLeft();
    const bool right = IsMovingRight();

    const bool canMove =
        state != PlayerState::Attack1 &&
        state != PlayerState::Attack2 &&
        state != PlayerState::Attack3 &&
        state != PlayerState::Hurt &&
        state != PlayerState::Dead;

    if (left != right && canMove)
    {
        movement.x = left ? -1.0f : 1.0f;
        GetSprite().SetFlipX(left);

        const float multiplier =
            IsRunningPressed() ? 1.75f : 1.0f;
        GetTransform().Translate(movement.x * speed * multiplier * deltaTime, 0.0f);
    }

    glm::vec2 position = GetTransform().GetPosition();
    position.x = glm::clamp(position.x, -640.0f, 512.0f);

    if (state == PlayerState::Jumping || state == PlayerState::Falling ||
        position.y > groundY)
    {
        verticalVelocity -= 1500.0f * deltaTime;
        position.y += verticalVelocity * deltaTime;

        if (verticalVelocity < 0.0f && state == PlayerState::Jumping)
            ChangeState(PlayerState::Falling);

        if (position.y <= groundY)
        {
            position.y = groundY;
            verticalVelocity = 0.0f;

            if (left || right)
                ChangeState(IsRunningPressed()
                    ? PlayerState::Running : PlayerState::Walking);
            else
                ChangeState(PlayerState::Idle);
        }
    }

    GetTransform().SetPosition(position);
}

void Player::UpdateAnimation()
{
    Animator& animator = GetAnimator();

    switch (state)
    {
    case PlayerState::Attack1:

        if (animator.IsFinished())
        {
            ChangeState(PlayerState::Idle);
        }

        break;

    case PlayerState::Attack2:

        if (animator.IsFinished())
        {
            ChangeState(PlayerState::Idle);
        }

        break;

    case PlayerState::Attack3:

        if (animator.IsFinished())
        {
            ChangeState(PlayerState::Idle);
        }

        break;

    case PlayerState::Hurt:

        if (animator.IsFinished())
        {
            ChangeState(PlayerState::Idle);
        }

        break;

    case PlayerState::Dead:

        //
        // Dead is a terminal state.
        //

        break;

    default:

        break;
    }
}

float Player::GetSpeed() const
{
    return speed;
}

void Player::SetSpeed(float speed)
{
    this->speed = speed;
}

bool Player::CanDealAttackDamage() const
{
    if (attackDamageApplied)
        return false;

    const std::size_t frame = GetAnimator().GetCurrentFrame();

    switch (state)
    {
    case PlayerState::Attack1:
        return frame >= 2;
    case PlayerState::Attack2:
        return frame >= 1;
    case PlayerState::Attack3:
        return frame >= 2;
    default:
        return false;
    }
}

void Player::ConfirmAttackDamage()
{
    attackDamageApplied = true;
}

void Player::ReceiveDamage()
{
    if (state == PlayerState::Dead)
        return;

    --health;

    if (health <= 0)
    {
        Kill();
        AudioPlayer::PlayPunch(true);
    }
    else
    {
        TakeDamage();
        AudioPlayer::PlayPunch(false);
    }
}

bool Player::IsDead() const
{
    return state == PlayerState::Dead;
}

int Player::GetHealth() const
{
    return health;
}

PlayerState Player::GetState() const
{
    return state;
}

bool Player::IsMoving() const
{
    return moving;
}

void Player::Stop()
{
    movement = glm::vec2(0.0f);

    moving = false;

    ChangeState(PlayerState::Idle);
}

void Player::Kill()
{
    ChangeState(PlayerState::Dead);
}

void Player::TakeDamage()
{
    if (state == PlayerState::Dead)
    {
        return;
    }

    ChangeState(PlayerState::Hurt);
}
