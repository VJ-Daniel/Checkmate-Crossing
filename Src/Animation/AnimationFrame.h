#pragma once

/// Identifies one sprite-sheet cell and how long it remains visible.
class AnimationFrame
{
public:

    AnimationFrame();

    AnimationFrame(
        int column,
        int row,
        float duration);

    int column;

    int row;

    float duration;
};
