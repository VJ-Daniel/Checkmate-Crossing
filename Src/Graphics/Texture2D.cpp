/*
    ============================================================
    Checkmate Crossing - Texture 2D

    Loads image pixels with stb_image, creates the OpenGL texture and
    owns its lifetime.

    Based on the Gangster Survival OpenGL framework by Leonardo Moura.
    ============================================================
*/

#include "Texture2D.h"

#include <iostream>

#include "stb_image.h"

namespace
{
    GLint ToGLWrap(TextureWrap wrap)
    {
        switch (wrap)
        {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        }

        return GL_CLAMP_TO_EDGE;
    }

    /// Minification filter. Mipmapped variants are only valid here, never on
    /// the magnification filter.
    GLint ToGLMinFilter(TextureFilter filter, bool hasMipmaps)
    {
        switch (filter)
        {
        case TextureFilter::Nearest:
            return GL_NEAREST;

        case TextureFilter::Linear:
            return GL_LINEAR;

        case TextureFilter::LinearMipmap:
            return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        }

        return GL_NEAREST;
    }

    GLint ToGLMagFilter(TextureFilter filter)
    {
        return (filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
    }
}

Texture2D::Texture2D()
    : id(0),
    width(0),
    height(0),
    channels(0)
{
}

Texture2D::~Texture2D()
{
    Destroy();
}

bool Texture2D::LoadFromFile(
    const std::string& filename,
    const TextureSettings& settings)
{
    Destroy();

    path = filename;

    stbi_set_flip_vertically_on_load(settings.flipVertically ? 1 : 0);

    // Passing 0 keeps the file's own channel count, so an opaque PNG is not
    // silently padded out to RGBA.
    unsigned char* pixels = stbi_load(
        filename.c_str(),
        &width,
        &height,
        &channels,
        0);

    if (pixels == nullptr)
    {
        const char* reason = stbi_failure_reason();

        std::cerr
            << "Texture2D: could not load \"" << filename << "\".\n"
            << "  reason: " << (reason ? reason : "unknown") << "\n"
            << "  check the path is relative to the working directory, which"
               " is the project root (where Src/ lives).\n";

        width = 0;
        height = 0;
        channels = 0;

        return false;
    }

    GLenum format = GL_RGBA;

    switch (channels)
    {
    case 1: format = GL_RED;  break;
    case 2: format = GL_RG;   break;
    case 3: format = GL_RGB;  break;
    case 4: format = GL_RGBA; break;

    default:
        std::cerr
            << "Texture2D: \"" << filename << "\" has "
            << channels << " channels, which is not supported.\n";

        stbi_image_free(pixels);

        width = 0;
        height = 0;
        channels = 0;

        return false;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Rows of a 1, 2 or 3 channel image are not necessarily 4-byte aligned,
    // which is what OpenGL assumes by default. Without this an odd-width RGB
    // image uploads skewed.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        pixels);

    if (settings.generateMipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);

    ApplyParameters(settings);

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    stbi_image_free(pixels);

    return true;
}

void Texture2D::ApplyParameters(const TextureSettings& settings) const
{
    const GLint wrap = ToGLWrap(settings.wrap);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        ToGLMinFilter(settings.filter, settings.generateMipmaps));

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        ToGLMagFilter(settings.filter));
}

void Texture2D::Bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2D::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::Destroy()
{
    if (id != 0)
    {
        glDeleteTextures(1, &id);
        id = 0;
    }

    width = 0;
    height = 0;
    channels = 0;
    path.clear();
}

bool Texture2D::IsValid() const
{
    return id != 0;
}

GLuint Texture2D::GetID() const
{
    return id;
}

int Texture2D::GetWidth() const
{
    return width;
}

int Texture2D::GetHeight() const
{
    return height;
}

int Texture2D::GetChannels() const
{
    return channels;
}

bool Texture2D::HasAlpha() const
{
    return channels == 2 || channels == 4;
}

const std::string& Texture2D::GetPath() const
{
    return path;
}
