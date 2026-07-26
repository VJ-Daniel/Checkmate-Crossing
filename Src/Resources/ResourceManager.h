#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Texture2D.h"
#include "SpriteSheet.h"

/// Shared cache for textures and sprite sheets addressed by stable names.
class ResourceManager
{
public:

    static bool Initialize();

    static void Shutdown();

    static std::shared_ptr<Texture2D> LoadTexture(
        const std::string& name,
        const std::string& filename);

    static std::shared_ptr<Texture2D> GetTexture(
        const std::string& name);

    static std::shared_ptr<SpriteSheet> CreateSpriteSheet(
        const std::string& name,
        const std::string& textureName,
        int columns,
        int rows);

    static std::shared_ptr<SpriteSheet> GetSpriteSheet(
        const std::string& name);

private:

    static std::unordered_map<
        std::string,
        std::shared_ptr<Texture2D>> textures;

    static std::unordered_map<
        std::string,
        std::shared_ptr<SpriteSheet>> spriteSheets;
};
