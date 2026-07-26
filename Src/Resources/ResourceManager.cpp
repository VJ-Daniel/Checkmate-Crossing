/*
    ============================================================
    Gangster Survival - Resource Manager
    Author: Leonardo Moura | Date: 2026

    Loads each texture once, caches it by name and creates shared
    sprite-sheet descriptions used by animations and UI elements.
    ============================================================
*/

#include "ResourceManager.h"

#include <utility>

std::unordered_map<
    std::string,
    std::shared_ptr<Texture2D>>
    ResourceManager::textures;

std::unordered_map<
    std::string,
    std::shared_ptr<SpriteSheet>>
    ResourceManager::spriteSheets;

bool ResourceManager::Initialize()
{
    textures.clear();
    spriteSheets.clear();

    return true;
}

void ResourceManager::Shutdown()
{
    spriteSheets.clear();
    textures.clear();
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture(
    const std::string& name,
    const std::string& filename)
{
    auto it = textures.find(name);

    if (it != textures.end())
    {
        return it->second;
    }

    auto texture = std::make_shared<Texture2D>();

    if (!texture->LoadFromFile(filename))
    {
        return nullptr;
    }

    textures.emplace(name, texture);

    return texture;
}

std::shared_ptr<Texture2D> ResourceManager::GetTexture(
    const std::string& name)
{
    auto it = textures.find(name);

    if (it == textures.end())
    {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<SpriteSheet> ResourceManager::CreateSpriteSheet(
    const std::string& name,
    const std::string& textureName,
    int columns,
    int rows)
{
    auto sheetIt = spriteSheets.find(name);

    if (sheetIt != spriteSheets.end())
    {
        return sheetIt->second;
    }

    auto texture = GetTexture(textureName);

    if (!texture)
    {
        return nullptr;
    }

    auto spriteSheet = std::make_shared<SpriteSheet>(
        texture,
        columns,
        rows);

    spriteSheets.emplace(name, spriteSheet);

    return spriteSheet;
}

std::shared_ptr<SpriteSheet> ResourceManager::GetSpriteSheet(
    const std::string& name)
{
    auto it = spriteSheets.find(name);

    if (it == spriteSheets.end())
    {
        return nullptr;
    }

    return it->second;
}
