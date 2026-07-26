/*
    ============================================================
    Gangster Survival - Shader Program
    Author: Leonardo Moura | Date: 2026

    Reads GLSL source files, compiles and links the OpenGL program,
    reports errors and exposes typed uniform setters.
    ============================================================
*/

#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <gtc/type_ptr.hpp>

Shader::Shader()
    : program(0)
{
}

Shader::~Shader()
{
    Destroy();
}

bool Shader::Load(
    const std::string& vertexPath,
    const std::string& fragmentPath)
{
    Destroy();

    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty())
    {
        return false;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    if (!CompileShader(vertexShader, vertexSource))
    {
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    if (!CompileShader(fragmentShader, fragmentSource))
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint success = GL_FALSE;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success);

    if (!success)
    {
        char log[1024];

        glGetProgramInfoLog(
            program,
            sizeof(log),
            nullptr,
            log);

        std::cerr
            << "Shader link error:\n"
            << log
            << std::endl;

        glDeleteProgram(program);
        program = 0;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return false;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Shader::Use() const
{
    glUseProgram(program);
}

void Shader::Destroy()
{
    if (program != 0)
    {
        glDeleteProgram(program);
        program = 0;
    }
}

GLuint Shader::GetProgram() const
{
    return program;
}

void Shader::SetBool(
    const std::string& name,
    bool value) const
{
    glUniform1i(
        glGetUniformLocation(program, name.c_str()),
        value);
}

void Shader::SetInt(
    const std::string& name,
    int value) const
{
    glUniform1i(
        glGetUniformLocation(program, name.c_str()),
        value);
}

void Shader::SetFloat(
    const std::string& name,
    float value) const
{
    glUniform1f(
        glGetUniformLocation(program, name.c_str()),
        value);
}

void Shader::SetVector2(
    const std::string& name,
    const glm::vec2& value) const
{
    glUniform2fv(
        glGetUniformLocation(program, name.c_str()),
        1,
        glm::value_ptr(value));
}

void Shader::SetVector3(
    const std::string& name,
    const glm::vec3& value) const
{
    glUniform3fv(
        glGetUniformLocation(program, name.c_str()),
        1,
        glm::value_ptr(value));
}

void Shader::SetVector4(
    const std::string& name,
    const glm::vec4& value) const
{
    glUniform4fv(
        glGetUniformLocation(program, name.c_str()),
        1,
        glm::value_ptr(value));
}

void Shader::SetMatrix4(
    const std::string& name,
    const glm::mat4& value) const
{
    glUniformMatrix4fv(
        glGetUniformLocation(program, name.c_str()),
        1,
        GL_FALSE,
        glm::value_ptr(value));
}

bool Shader::CompileShader(
    GLuint shader,
    const std::string& source)
{
    const char* code = source.c_str();

    glShaderSource(
        shader,
        1,
        &code,
        nullptr);

    glCompileShader(shader);

    GLint success = GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success);

    if (!success)
    {
        char log[1024];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            nullptr,
            log);

        std::cerr
            << "Shader compilation error:\n"
            << log
            << std::endl;

        return false;
    }

    return true;
}

std::string Shader::ReadFile(
    const std::string& filename) const
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr
            << "Unable to open shader file: "
            << filename
            << std::endl;

        return "";
    }

    std::stringstream stream;

    stream << file.rdbuf();

    return stream.str();
}
