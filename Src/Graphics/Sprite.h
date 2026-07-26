#pragma once

#include <memory>

#include <glm.hpp>

#include "SpriteSheet.h"

/// Renderable sprite state: sheet, frame, tint, flips and visibility.
class Sprite
{
public:

    Sprite();

    explicit Sprite(std::shared_ptr<SpriteSheet> spriteSheet);

    void SetSpriteSheet(std::shared_ptr<SpriteSheet> spriteSheet);

    std::shared_ptr<SpriteSheet> GetSpriteSheet() const;

    void SetFrame(
        int column,
        int row);

    void SetTint(const glm::vec4& color);

    const glm::vec4& GetTint() const;

    void SetFlipX(bool flip);

    bool GetFlipX() const;

    void SetFlipY(bool flip);

    bool GetFlipY() const;

    void SetVisible(bool visible);

    bool IsVisible() const;

    int GetColumn() const;

    int GetRow() const;

private:

    std::shared_ptr<SpriteSheet> spriteSheet;

    glm::vec4 tint;

    int column;

    int row;

    bool flipX;

    bool flipY;

    bool visible;
};
