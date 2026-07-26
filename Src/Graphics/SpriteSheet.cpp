/*
    ============================================================
    Gangster Survival - Sprite Sheet
    Author: Leonardo Moura | Date: 2026

    Divides one texture into a column/row grid and converts frame
    coordinates into normalized OpenGL UV coordinates.
    ============================================================
*/

#include "SpriteSheet.h"

SpriteSheet::SpriteSheet()
    : texture(nullptr),
    columns(1),
    rows(1)
{
}

SpriteSheet::SpriteSheet(
    std::shared_ptr<Texture2D> texture,
    int columns,
    int rows)
    : texture(texture),
    columns(columns),
    rows(rows)
{
}

void SpriteSheet::SetTexture(std::shared_ptr<Texture2D> texture)
{
    this->texture = texture;
}

std::shared_ptr<Texture2D> SpriteSheet::GetTexture() const
{
    return texture;
}

void SpriteSheet::SetGrid(
    int columns,
    int rows)
{
    this->columns = columns;
    this->rows = rows;
}

int SpriteSheet::GetColumns() const
{
    return columns;
}

int SpriteSheet::GetRows() const
{
    return rows;
}

glm::vec2 SpriteSheet::GetFrameSize() const
{
    if (!texture)
        return glm::vec2(0.0f);

    return glm::vec2(
        static_cast<float>(texture->GetWidth()) / columns,
        static_cast<float>(texture->GetHeight()) / rows);
}

void SpriteSheet::GetUV(
    int column,
    int row,
    glm::vec2& uvMin,
    glm::vec2& uvMax) const
{
    if (!texture)
    {
        uvMin = glm::vec2(0.0f);
        uvMax = glm::vec2(1.0f);
        return;
    }

    float cellWidth = 1.0f / static_cast<float>(columns);
    float cellHeight = 1.0f / static_cast<float>(rows);

    uvMin.x = column * cellWidth;
    uvMin.y = row * cellHeight;

    uvMax.x = uvMin.x + cellWidth;
    uvMax.y = uvMin.y + cellHeight;
}
