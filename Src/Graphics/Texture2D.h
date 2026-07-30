#pragma once

#include <string>

#include <glew.h>

/// How a texture is sampled when it is drawn larger or smaller than its
/// pixels.
///
/// Nearest is the default because this project's art is blocky: linear
/// filtering on pixel art blurs the edges and, on a sprite sheet, bleeds
/// neighbouring frames into each other.
enum class TextureFilter
{
    Nearest,
    Linear,

    /// Linear with mipmaps. Only worth it for large textures drawn at many
    /// different sizes; mipmaps make atlas bleeding worse, so avoid it for
    /// sprite sheets.
    LinearMipmap
};

/// What happens when a UV falls outside 0..1.
enum class TextureWrap
{
    /// The safe default for sprites. Repeat lets a frame at the edge of an
    /// atlas sample the opposite edge and show a seam.
    ClampToEdge,

    Repeat,
    MirroredRepeat
};

/// Everything about how a texture is created, in one place.
struct TextureSettings
{
    TextureFilter filter = TextureFilter::Nearest;

    TextureWrap wrap = TextureWrap::ClampToEdge;

    /// Flip the image rows as they are loaded.
    ///
    /// Image files store rows top-first, but OpenGL treats the first row it
    /// is given as v = 0. Flipping on load makes v = 0 the bottom of the
    /// picture, so v runs upwards and a quad whose UVs run 0..1 bottom to
    /// top shows the image the right way up.
    ///
    /// The whole project assumes this is true. SpriteRegion's pixel helpers
    /// convert top-left pixel coordinates for it.
    bool flipVertically = true;

    bool generateMipmaps = false;
};

/// RAII wrapper for one OpenGL 2D texture loaded with stb_image.
///
/// The object owns its GL texture name and deletes it on destruction, so it
/// must be released while a GL context is still current. In practice textures
/// are held by ResourceManager, which the game clears during shutdown.
class Texture2D
{
public:

    Texture2D();

    ~Texture2D();

    // Owning a GL handle, so copying would delete it twice.
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    /// Loads a PNG or JPG. Returns false and leaves the object empty if the
    /// file is missing, unreadable or has an unsupported channel count.
    bool LoadFromFile(
        const std::string& filename,
        const TextureSettings& settings = TextureSettings());

    void Bind(GLuint unit = 0) const;

    void Unbind() const;

    void Destroy();

    bool IsValid() const;

    GLuint GetID() const;

    int GetWidth() const;

    int GetHeight() const;

    /// Channels in the source file: 1 grey, 2 grey+alpha, 3 RGB, 4 RGBA.
    int GetChannels() const;

    bool HasAlpha() const;

    /// Path this texture was loaded from, for diagnostics.
    const std::string& GetPath() const;

private:

    void ApplyParameters(const TextureSettings& settings) const;

    GLuint id;

    int width;

    int height;

    int channels;

    std::string path;
};
