#pragma once

#include <memory>

#include <glm.hpp>

#include "Sprite.h"
#include "Texture2D.h"

/// Describes a texture as a regular grid of frames.
///
/// This is a convenience layer over SpriteRegion for the case where a sheet's
/// grid is fixed and worth naming once: it pairs the texture with its layout
/// so callers ask for "column 2, row 1" without repeating the grid size.
///
/// All of its UV maths goes through SpriteRegion, so the whole project has one
/// convention: column 0, row 0 is the sheet's TOP-LEFT frame.
class SpriteSheet
{
public:

    SpriteSheet();

    SpriteSheet(
        std::shared_ptr<Texture2D> texture,
        int columns,
        int rows);

    void SetTexture(std::shared_ptr<Texture2D> texture);

    std::shared_ptr<Texture2D> GetTexture() const;

    void SetGrid(
        int columns,
        int rows);

    int GetColumns() const;

    int GetRows() const;

    /// Frame size in pixels.
    glm::vec2 GetFrameSize() const;

    /// UV region of one frame. Column 0, row 0 is the top-left frame.
    SpriteRegion GetRegion(
        int column,
        int row) const;

    void GetUV(
        int column,
        int row,
        glm::vec2& uvMin,
        glm::vec2& uvMax) const;

private:

    std::shared_ptr<Texture2D> texture;

    int columns;

    int rows;
};
