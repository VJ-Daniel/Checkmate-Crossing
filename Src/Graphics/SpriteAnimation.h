#pragma once

#include <cstddef>
#include <vector>

#include "Sprite.h"

/// Plays a list of sprite regions over time.
///
/// It holds regions, never textures, so the animation is pure state. Ten
/// objects can each own their own SpriteAnimation over the same sprite sheet
/// and sit on different frames, and nothing about the texture is duplicated.
///
/// Deliberately small: a frame list, a clock and the playback flags. Anything
/// more - blending, events, state machines - belongs in whatever gameplay code
/// eventually needs it, not here.
class SpriteAnimation
{
public:

    SpriteAnimation();

    /// Appends one frame. Duration is in seconds and is clamped to a small
    /// positive value, so a zero never turns Update into an infinite loop.
    void AddFrame(
        const SpriteRegion& region,
        float duration);

    /// Fills the animation from one row of a grid sheet, left to right.
    /// A convenience for the overwhelmingly common case.
    void AddFramesFromGridRow(
        int row,
        int firstColumn,
        int frameCount,
        int columns,
        int rows,
        float frameDuration);

    void Clear();

    //---------------------------------------------------------
    // Playback
    //---------------------------------------------------------

    /// Starts or resumes. On a finished one-shot this restarts from frame 0.
    void Play();

    void Pause();

    /// Stops and rewinds to the first frame.
    void Stop();

    /// Rewinds to the first frame and keeps playing.
    void Restart();

    /// Advances the clock. Does nothing when paused, stopped or finished.
    void Update(float deltaTime);

    void SetLooping(bool looping);

    bool IsLooping() const;

    bool IsPlaying() const;

    /// True once a non-looping animation has shown its last frame. A looping
    /// animation never finishes.
    bool IsFinished() const;

    //---------------------------------------------------------
    // Current state
    //---------------------------------------------------------

    int GetCurrentFrame() const;

    /// Region of the current frame, or the whole texture when there are no
    /// frames, so a sprite driven by an empty animation still draws.
    const SpriteRegion& GetCurrentRegion() const;

    std::size_t GetFrameCount() const;

    /// Copies the current frame's region onto a sprite. The usual way to
    /// drive a Sprite from an animation each frame.
    void ApplyTo(Sprite& sprite) const;

private:

    struct Frame
    {
        SpriteRegion region;

        float duration = 0.1f;
    };

    std::vector<Frame> frames;

    int currentFrame;

    float frameTimer;

    bool looping;

    bool playing;

    bool finished;
};
