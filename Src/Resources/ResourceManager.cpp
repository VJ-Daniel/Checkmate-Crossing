/*
    ============================================================
    Checkmate Crossing - Resource Manager

    Loads each texture once, caches it by name, and describes textures
    as sprite-sheet grids. Sole owner of loaded GPU textures.

    Based on the Gangster Survival OpenGL framework by Leonardo Moura.
    ============================================================
*/

#include "ResourceManager.h"

#include <iostream>
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
    // Sheets first: they hold texture references, and clearing them lets the
    // textures actually be released when the texture map is cleared next.
    spriteSheets.clear();
    textures.clear();
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture(
    const std::string& name,
    const std::string& filename,
    const TextureSettings& settings)
{
    auto it = textures.find(name);

    if (it != textures.end())
        return it->second;

    auto texture = std::make_shared<Texture2D>();

    if (!texture->LoadFromFile(filename, settings))
    {
        // Nothing is cached on failure, so a later call can retry once the
        // file exists rather than being stuck with a broken entry.
        std::cerr
            << "ResourceManager: texture \"" << name
            << "\" was not registered.\n";

        return nullptr;
    }

    textures.emplace(name, std::move(texture));

    return textures[name];
}

std::shared_ptr<Texture2D> ResourceManager::GetTexture(
    const std::string& name)
{
    auto it = textures.find(name);

    if (it == textures.end())
        return nullptr;

    return it->second;
}

bool ResourceManager::HasTexture(const std::string& name)
{
    return textures.find(name) != textures.end();
}

void ResourceManager::UnloadTexture(const std::string& name)
{
    textures.erase(name);
}

std::size_t ResourceManager::GetTextureCount()
{
    return textures.size();
}

std::shared_ptr<SpriteSheet> ResourceManager::CreateSpriteSheet(
    const std::string& name,
    const std::string& textureName,
    int columns,
    int rows)
{
    auto sheetIt = spriteSheets.find(name);

    if (sheetIt != spriteSheets.end())
        return sheetIt->second;

    auto texture = GetTexture(textureName);

    if (!texture)
    {
        std::cerr
            << "ResourceManager: cannot build sprite sheet \"" << name
            << "\" because texture \"" << textureName
            << "\" is not loaded.\n";

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
        return nullptr;

    return it->second;
}
