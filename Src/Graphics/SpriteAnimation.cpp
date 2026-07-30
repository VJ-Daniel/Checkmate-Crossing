/*
    ============================================================
    Checkmate Crossing - Sprite Animation

    Frame timing and playback state for a sprite sheet. Holds regions
    only, so the same sheet can drive many animations at once.
    ============================================================
*/

#include "SpriteAnimation.h"

#include <algorithm>

namespace
{
    /// Shortest frame the clock will accept. A zero-length frame would let
    /// Update spin without ever consuming its remaining time.
    constexpr float MinFrameDuration = 0.001f;

    const SpriteRegion FallbackRegion = SpriteRegion::Full();
}

SpriteAnimation::SpriteAnimation()
    : currentFrame(0),
    frameTimer(0.0f),
    looping(true),
    playing(false),
    finished(false)
{
}

void SpriteAnimation::AddFrame(
    const SpriteRegion& region,
    float duration)
{
    Frame frame;

    frame.region = region;
    frame.duration = std::max(duration, MinFrameDuration);

    frames.push_back(frame);
}

void SpriteAnimation::AddFramesFromGridRow(
    int row,
    int firstColumn,
    int frameCount,
    int columns,
    int rows,
    float frameDuration)
{
    for (int index = 0; index < frameCount; ++index)
    {
        AddFrame(
            SpriteRegion::FromGridCounts(
                firstColumn + index,
                row,
                columns,
                rows),
            frameDuration);
    }
}

void SpriteAnimation::Clear()
{
    frames.clear();

    currentFrame = 0;
    frameTimer = 0.0f;
    playing = false;
    finished = false;
}

void SpriteAnimation::Play()
{
    // Replaying a finished one-shot is the common intent, so treat it as a
    // restart rather than doing nothing.
    if (finished)
    {
        currentFrame = 0;
        frameTimer = 0.0f;
        finished = false;
    }

    playing = true;
}

void SpriteAnimation::Pause()
{
    playing = false;
}

void SpriteAnimation::Stop()
{
    playing = false;
    finished = false;
    currentFrame = 0;
    frameTimer = 0.0f;
}

void SpriteAnimation::Restart()
{
    currentFrame = 0;
    frameTimer = 0.0f;
    finished = false;
    playing = true;
}

void SpriteAnimation::Update(float deltaTime)
{
    if (!playing || finished || frames.size() < 2)
        return;

    frameTimer += deltaTime;

    // A while loop rather than a single step, so a long frame time cannot
    // leave the animation lagging behind the clock.
    while (frameTimer >= frames[currentFrame].duration)
    {
        frameTimer -= frames[currentFrame].duration;

        const int lastFrame = static_cast<int>(frames.size()) - 1;

        if (currentFrame < lastFrame)
        {
            ++currentFrame;
            continue;
        }

        if (looping)
        {
            currentFrame = 0;
            continue;
        }

        // A one-shot holds its last frame and reports that it is done.
        finished = true;
        playing = false;
        frameTimer = 0.0f;
        break;
    }
}

void SpriteAnimation::SetLooping(bool looping)
{
    this->looping = looping;
}

bool SpriteAnimation::IsLooping() const
{
    return looping;
}

bool SpriteAnimation::IsPlaying() const
{
    return playing;
}

bool SpriteAnimation::IsFinished() const
{
    return finished;
}

int SpriteAnimation::GetCurrentFrame() const
{
    return currentFrame;
}

const SpriteRegion& SpriteAnimation::GetCurrentRegion() const
{
    if (frames.empty())
        return FallbackRegion;

    return frames[static_cast<std::size_t>(currentFrame)].region;
}

std::size_t SpriteAnimation::GetFrameCount() const
{
    return frames.size();
}

void SpriteAnimation::ApplyTo(Sprite& sprite) const
{
    sprite.region = GetCurrentRegion();
}
