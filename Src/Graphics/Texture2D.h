#pragma once

#include <string>

#include <glew.h>

/// RAII wrapper for loading and binding an OpenGL 2D texture with stb_image.
class Texture2D
{
public:

    Texture2D();

    ~Texture2D();

    bool LoadFromFile(
        const std::string& filename,
        bool flipVertically = false);

    void Bind(
        GLuint unit = 0) const;

    void Unbind() const;

    void Destroy();

    GLuint GetID() const;

    int GetWidth() const;

    int GetHeight() const;

    int GetChannels() const;

private:

    GLuint id;

    int width;

    int height;

    int channels;
};
