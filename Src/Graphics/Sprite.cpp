/*
    ============================================================
    Checkmate Crossing - Sprite

    The sprite data structure and the one place where image pixel
    coordinates are converted into OpenGL UV coordinates.
    ============================================================
*/

#include "Sprite.h"

#include <algorithm>

SpriteRegion SpriteRegion::Full()
{
    SpriteRegion region;

    region.min = glm::vec2(0.0f, 0.0f);
    region.max = glm::vec2(1.0f, 1.0f);

    return region;
}

SpriteRegion SpriteRegion::FromPixels(
    int x,
    int y,
    int width,
    int height,
    int textureWidth,
    int textureHeight)
{
    SpriteRegion region;

    if (textureWidth <= 0 || textureHeight <= 0)
        return region;

    const float texW = static_cast<float>(textureWidth);
    const float texH = static_cast<float>(textureHeight);

    // U runs left to right, the same direction as pixel x.
    region.min.x = static_cast<float>(x) / texW;
    region.max.x = static_cast<float>(x + width) / texW;

    // V is inverted relative to pixel y, because the texture was flipped when
    // it was loaded. The pixel row nearest the top of the image is the one
    // with the largest v.
    region.min.y = 1.0f - static_cast<float>(y + height) / texH;
    region.max.y = 1.0f - static_cast<float>(y) / texH;

    return region;
}

SpriteRegion SpriteRegion::FromGrid(
    int column,
    int row,
    int frameWidth,
    int frameHeight,
    int textureWidth,
    int textureHeight)
{
    return FromPixels(
        column * frameWidth,
        row * frameHeight,
        frameWidth,
        frameHeight,
        textureWidth,
        textureHeight);
}

SpriteRegion SpriteRegion::FromGridCounts(
    int column,
    int row,
    int columns,
    int rows)
{
    SpriteRegion region;

    if (columns <= 0 || rows <= 0)
        return region;

    const float cellW = 1.0f / static_cast<float>(columns);
    const float cellH = 1.0f / static_cast<float>(rows);

    region.min.x = static_cast<float>(column) * cellW;
    region.max.x = region.min.x + cellW;

    // Row 0 is the top of the sheet, so it maps to the highest v band.
    region.max.y = 1.0f - static_cast<float>(row) * cellH;
    region.min.y = region.max.y - cellH;

    return region;
}

glm::vec2 SpriteRegion::GetSize() const
{
    return max - min;
}

Sprite Sprite::CreateScreen(
    const std::string& textureName,
    const glm::vec2& pixelPosition,
    const glm::vec2& pixelSize)
{
    Sprite sprite;

    sprite.textureName = textureName;
    sprite.position = glm::vec3(pixelPosition, 0.0f);
    sprite.size = pixelSize;
    sprite.mode = SpriteRenderMode::Screen;

    // Screen space runs top corner first (local y = 0 is the sprite's top
    // edge), but a texture's v = 0 is its bottom row (see the flip note on
    // SpriteRegion::FromPixels). Left uncorrected, every Screen sprite draws
    // its source image upside down; flipping here once means every caller
    // of CreateScreen sees their art right-side up without having to think
    // about it.
    sprite.flipY = true;

    return sprite;
}

Sprite Sprite::CreateBillboard(
    const std::string& textureName,
    const glm::vec3& worldPosition,
    const glm::vec2& worldSize)
{
    Sprite sprite;

    sprite.textureName = textureName;
    sprite.position = worldPosition;
    sprite.size = worldSize;
    sprite.mode = SpriteRenderMode::Billboard;

    return sprite;
}

Sprite Sprite::CreateGroundDecal(
    const std::string& textureName,
    const glm::vec3& groundPosition,
    const glm::vec2& worldSize)
{
    Sprite sprite;

    sprite.textureName = textureName;
    sprite.position = groundPosition;
    sprite.size = worldSize;
    sprite.mode = SpriteRenderMode::GroundDecal;

    return sprite;
}
