/*
    ============================================================
    Gangster Survival - Texture 2D
    Author: Leonardo Moura | Date: 2026

    Loads image pixels with stb_image, creates the OpenGL texture,
    configures sampling/mipmaps and manages GPU resource lifetime.
    ============================================================
*/

#include "Texture2D.h"

#include <iostream>

#include "stb_image.h"

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
    bool flipVertically)
{
    Destroy();

    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* pixels =
        stbi_load(
            filename.c_str(),
            &width,
            &height,
            &channels,
            0);

    if (pixels == nullptr)
    {
        std::cerr
            << "Failed to load texture: "
            << filename
            << std::endl;

        return false;
    }

    GLenum format = GL_RGB;

    switch (channels)
    {
    case 1:
        format = GL_RED;
        break;

    case 3:
        format = GL_RGB;
        break;

    case 4:
        format = GL_RGBA;
        break;

    default:
        stbi_image_free(pixels);
        return false;
    }

    glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR);

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

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    return true;
}

void Texture2D::Bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);

    glBindTexture(
        GL_TEXTURE_2D,
        id);
}

void Texture2D::Unbind() const
{
    glBindTexture(
        GL_TEXTURE_2D,
        0);
}

void Texture2D::Destroy()
{
    if (id != 0)
    {
        glDeleteTextures(
            1,
            &id);

        id = 0;
    }

    width = 0;
    height = 0;
    channels = 0;
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
