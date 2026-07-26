/*
    ============================================================
    Gangster Survival - Animator
    Author: Leonardo Moura | Date: 2026

    Advances clips with delta time, applies frames to a Sprite and
    tracks playing, paused, looping and finished states.
    ============================================================
*/

#include "Animator.h"

Animator::Animator()
    : sprite(nullptr),
    currentClip(nullptr),
    currentFrame(0),
    elapsedTime(0.0f),
    playing(false),
    paused(false),
    finished(false)
{
}

void Animator::SetSprite(Sprite* sprite)
{
    this->sprite = sprite;
}

Sprite* Animator::GetSprite() const
{
    return sprite;
}

void Animator::Play(
    const std::shared_ptr<AnimationClip>& clip,
    bool restart)
{
    if (!clip)
    {
        return;
    }

    if (!restart && currentClip == clip && playing)
    {
        return;
    }

    currentClip = clip;

    currentFrame = 0;

    elapsedTime = 0.0f;

    playing = true;

    paused = false;

    finished = false;

    if (sprite)
    {
        sprite->SetSpriteSheet(
            currentClip->GetSpriteSheet());
    }

    ApplyCurrentFrame();
}

void Animator::Stop()
{
    playing = false;

    paused = false;

    finished = true;

    currentFrame = 0;

    elapsedTime = 0.0f;
}

void Animator::Pause()
{
    if (playing)
    {
        paused = true;
    }
}

void Animator::Resume()
{
    if (playing)
    {
        paused = false;
    }
}

void Animator::Update(float deltaTime)
{
    if (!playing)
        return;

    if (paused)
        return;

    if (!currentClip)
        return;

    if (currentClip->GetFrameCount() == 0)
        return;

    elapsedTime += deltaTime;

    const AnimationFrame& frame =
        currentClip->GetFrame(currentFrame);

    if (elapsedTime < frame.duration)
        return;

    elapsedTime = 0.0f;

    currentFrame++;

    if (currentFrame >= currentClip->GetFrameCount())
    {
        if (currentClip->IsLooping())
        {
            currentFrame = 0;
        }
        else
        {
            currentFrame =
                currentClip->GetFrameCount() - 1;

            playing = false;

            finished = true;
        }
    }

    ApplyCurrentFrame();
}

bool Animator::IsPlaying() const
{
    return playing;
}

bool Animator::IsPaused() const
{
    return paused;
}

bool Animator::IsFinished() const
{
    return finished;
}

std::shared_ptr<AnimationClip> Animator::GetCurrentClip() const
{
    return currentClip;
}

std::size_t Animator::GetCurrentFrame() const
{
    return currentFrame;
}

void Animator::ApplyCurrentFrame()
{
    if (!sprite)
        return;

    if (!currentClip)
        return;

    if (currentClip->GetFrameCount() == 0)
        return;

    const AnimationFrame& frame =
        currentClip->GetFrame(currentFrame);

    sprite->SetFrame(
        frame.column,
        frame.row);
}
