#pragma once

#include <memory>
#include <vector>

#include "AnimationFrame.h"
#include "SpriteSheet.h"

/// Ordered collection of sprite-sheet frames with optional looping.
class AnimationClip
{
public:

    explicit AnimationClip(bool looping = true);

    void AddFrame(const AnimationFrame& frame);

    const AnimationFrame& GetFrame(size_t index) const;

    size_t GetFrameCount() const;

    void SetLooping(bool looping);

    bool IsLooping() const;

    void SetSpriteSheet(std::shared_ptr<SpriteSheet> spriteSheet);

    std::shared_ptr<SpriteSheet> GetSpriteSheet() const;

private:

    std::shared_ptr<SpriteSheet> spriteSheet;

    std::vector<AnimationFrame> frames;

    bool looping;
};
