#pragma once

#include <memory>

#include <glm.hpp>

#include "Texture2D.h"

/// Divides a texture into a regular grid and calculates frame UV bounds.
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

    glm::vec2 GetFrameSize() const;

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
