/*
    ============================================================
    Gangster Survival - Animation Frame
    Author: Leonardo Moura | Date: 2026

    Represents one animation step through a sprite-sheet column,
    row and duration measured in seconds.
    ============================================================
*/

#include "AnimationFrame.h"

AnimationFrame::AnimationFrame()
    : column(0),
    row(0),
    duration(0.1f)
{
}

AnimationFrame::AnimationFrame(
    int column,
    int row,
    float duration)
    : column(column),
    row(row),
    duration(duration)
{
}
