#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "SpriteSheet.h"
#include "Texture2D.h"

/// Shared cache for textures and sprite sheets, addressed by stable names.
///
/// ---------------------------------------------------------------------
/// Ownership
/// ---------------------------------------------------------------------
/// This is the only owner of a loaded texture. Everything else refers to one
/// by name: a Sprite stores a std::string, not a pointer, so a sprite can be
/// copied, stored and thrown away with no effect on GPU resources.
///
/// Each Texture2D deletes its GL texture in its destructor, which means the
/// cache has to be emptied while a GL context is still current. Shutdown does
/// that, and Game::Shutdown calls it before the window goes away. The static
/// maps would otherwise be destroyed at program exit, after the context is
/// gone, and the deletes would be issued against nothing.
class ResourceManager
{
public:

    static bool Initialize();

    static void Shutdown();

    //---------------------------------------------------------
    // Textures
    //---------------------------------------------------------

    /// Loads an image and caches it under a name.
    ///
    /// A name that is already loaded is returned as-is and the file is not
    /// touched, so calling this repeatedly is cheap and safe - which is what
    /// lets several sprites share one texture without coordinating.
    ///
    /// Returns nullptr if the file could not be loaded; Texture2D prints why.
    static std::shared_ptr<Texture2D> LoadTexture(
        const std::string& name,
        const std::string& filename,
        const TextureSettings& settings = TextureSettings());

    /// Returns a cached texture, or nullptr if that name was never loaded.
    static std::shared_ptr<Texture2D> GetTexture(
        const std::string& name);

    static bool HasTexture(const std::string& name);

    /// Drops the cache's reference. The GL texture goes away once every other
    /// holder has released it too.
    static void UnloadTexture(const std::string& name);

    static std::size_t GetTextureCount();

    //---------------------------------------------------------
    // Sprite sheets
    //---------------------------------------------------------

    /// Describes an already-loaded texture as a grid of frames.
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
