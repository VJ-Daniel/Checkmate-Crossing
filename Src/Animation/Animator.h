#pragma once

#include <memory>

#include "AnimationClip.h"
#include "Sprite.h"

/// Advances AnimationClip frames and writes them to an attached Sprite.
class Animator
{
public:

    Animator();

    void SetSprite(Sprite* sprite);

    Sprite* GetSprite() const;

    /// Selects a clip; restart forces playback even if it is already active.
    void Play(
        const std::shared_ptr<AnimationClip>& clip,
        bool restart = false);

    void Stop();

    void Pause();

    void Resume();

    /// Advances the current frame according to its configured duration.
    void Update(float deltaTime);

    bool IsPlaying() const;

    bool IsPaused() const;

    /// Reports completion of a non-looping animation.
    bool IsFinished() const;

    std::shared_ptr<AnimationClip> GetCurrentClip() const;

    std::size_t GetCurrentFrame() const;

private:

    void ApplyCurrentFrame();

private:

    Sprite* sprite;

    std::shared_ptr<AnimationClip> currentClip;

    std::size_t currentFrame;

    float elapsedTime;

    bool playing;

    bool paused;

    bool finished;
};
