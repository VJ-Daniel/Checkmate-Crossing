/*
    ============================================================
    Gangster Survival - Sprite State
    Author: Leonardo Moura | Date: 2026

    Stores the selected sprite sheet/frame plus tint, horizontal or
    vertical flips and visibility used during rendering.
    ============================================================
*/

#include "Sprite.h"

Sprite::Sprite()
    : spriteSheet(nullptr),
    tint(1.0f, 1.0f, 1.0f, 1.0f),
    column(0),
    row(0),
    flipX(false),
    flipY(false),
    visible(true)
{
}

Sprite::Sprite(std::shared_ptr<SpriteSheet> spriteSheet)
    : spriteSheet(spriteSheet),
    tint(1.0f, 1.0f, 1.0f, 1.0f),
    column(0),
    row(0),
    flipX(false),
    flipY(false),
    visible(true)
{
}

void Sprite::SetSpriteSheet(std::shared_ptr<SpriteSheet> spriteSheet)
{
    this->spriteSheet = spriteSheet;
}

std::shared_ptr<SpriteSheet> Sprite::GetSpriteSheet() const
{
    return spriteSheet;
}

void Sprite::SetFrame(
    int column,
    int row)
{
    this->column = column;
    this->row = row;
}

void Sprite::SetTint(const glm::vec4& color)
{
    tint = color;
}

const glm::vec4& Sprite::GetTint() const
{
    return tint;
}

void Sprite::SetFlipX(bool flip)
{
    flipX = flip;
}

bool Sprite::GetFlipX() const
{
    return flipX;
}

void Sprite::SetFlipY(bool flip)
{
    flipY = flip;
}

bool Sprite::GetFlipY() const
{
    return flipY;
}

void Sprite::SetVisible(bool visible)
{
    this->visible = visible;
}

bool Sprite::IsVisible() const
{
    return visible;
}

int Sprite::GetColumn() const
{
    return column;
}

int Sprite::GetRow() const
{
    return row;
}
