#pragma once

#include <string>

#include <glm.hpp>
#include <glew.h>

/// Compiles, links and configures the GLSL program used by sprite rendering.
class Shader
{
public:

    Shader();

    ~Shader();

    bool Load(
        const std::string& vertexPath,
        const std::string& fragmentPath);

    void Use() const;

    void Destroy();

    GLuint GetProgram() const;

    void SetBool(
        const std::string& name,
        bool value) const;

    void SetInt(
        const std::string& name,
        int value) const;

    void SetFloat(
        const std::string& name,
        float value) const;

    void SetVector2(
        const std::string& name,
        const glm::vec2& value) const;

    void SetVector3(
        const std::string& name,
        const glm::vec3& value) const;

    void SetVector4(
        const std::string& name,
        const glm::vec4& value) const;

    void SetMatrix4(
        const std::string& name,
        const glm::mat4& value) const;

private:

    bool CompileShader(
        GLuint shader,
        const std::string& source);

    std::string ReadFile(
        const std::string& filename) const;

private:

    GLuint program;
};
