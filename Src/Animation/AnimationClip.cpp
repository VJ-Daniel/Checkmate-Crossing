/*
    ============================================================
    Gangster Survival - Animation Clip
    Author: Leonardo Moura | Date: 2026

    Stores an ordered timeline of sprite-sheet frames, the sheet
    used by those frames and whether playback loops at the end.
    ============================================================
*/

#include "AnimationClip.h"

AnimationClip::AnimationClip(bool looping)
    : looping(looping)
{
}

void AnimationClip::AddFrame(const AnimationFrame& frame)
{
    frames.push_back(frame);
}

const AnimationFrame& AnimationClip::GetFrame(size_t index) const
{
    return frames.at(index);
}

size_t AnimationClip::GetFrameCount() const
{
    return frames.size();
}

void AnimationClip::SetLooping(bool looping)
{
    this->looping = looping;
}

bool AnimationClip::IsLooping() const
{
    return looping;
}

void AnimationClip::SetSpriteSheet(
    std::shared_ptr<SpriteSheet> spriteSheet)
{
    this->spriteSheet = spriteSheet;
}

std::shared_ptr<SpriteSheet> AnimationClip::GetSpriteSheet() const
{
    return spriteSheet;
}
