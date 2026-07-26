/*
    ============================================================
    Gangster Survival - OpenGL

    Author: Leonardo Moura
    Date: 2026

    Description:

    This application demonstrates the fundamentals of modern
    OpenGL by implementing an animated 2D survival game.

    The project initializes an OpenGL rendering context, loads
    shaders, textures, sprite sheets and audio, and manages an
    interactive real-time game loop with enemies and a HUD.

    The player can walk, run, jump and perform three melee attacks
    using the keyboard or a compatible gamepad.

    Features:

        - OpenGL and GLFW window initialization
        - GLEW function loading
        - Orthographic 2D camera
        - Animated sprite sheets
        - Delta Time movement and jumping
        - Keyboard and gamepad input
        - Enemy AI, combat and spawning
        - Health HUD and Game Over screen
        - Music and sound effects
        - Restartable real-time game loop

    Technologies:

        - OpenGL 3.3 Core Profile
        - GLFW
        - GLEW
        - GLM
        - stb_image
        - Windows Multimedia API
        - C++

    Learning Topics:

        - Rendering Pipeline
        - Window and Context Management
        - Model / View / Projection Matrices
        - Texture and Sprite Rendering
        - Animation State Machines
        - User Input
        - Basic Enemy AI and Collision
        - Audio Playback
        - UI and Game State Architecture

    ============================================================
*/

#include <algorithm>
#include <iostream>

#include <glew.h>
#include <glfw3.h>

#include "Game.h"
#include "Input.h"

namespace
{
    constexpr int ScreenWidth = 1280;
    constexpr int ScreenHeight = 720;

    GLFWwindow* window = nullptr;
    Game game;

    void FramebufferSizeCallback(GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
        game.GetCamera().SetViewport(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    bool InitializeWindow()
    {
        if (!glfwInit())
        {
            std::cerr << "Erro ao inicializar GLFW.\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(
            ScreenWidth, ScreenHeight, "Gangster Run 2D", nullptr, nullptr);

        if (!window)
        {
            std::cerr << "Erro ao criar a janela.\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "Erro ao inicializar GLEW.\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }

        glViewport(0, 0, ScreenWidth, ScreenHeight);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return true;
    }
}

int main()
{
    if (!InitializeWindow())
        return -1;

    Input::Initialize(window);

    if (!game.Initialize(
        static_cast<float>(ScreenWidth),
        static_cast<float>(ScreenHeight)))
    {
        std::cerr << "Erro ao inicializar o jogo.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        Input::Update();

        if (Input::IsKeyPressed(Key::Escape) ||
            Input::IsGamepadButtonPressed(GamepadButton::Start))
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (game.IsGameOver() && Input::IsKeyPressed(Key::Enter))
            game.Restart();

        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(
            std::min(currentTime - previousTime, 0.05));
        previousTime = currentTime;

        game.Update(deltaTime);

        glClearColor(0.04f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        game.Render();

        glfwSwapBuffers(window);
    }

    game.Shutdown();
    glfwDestroyWindow(window);
    window = nullptr;
    glfwTerminate();
    return 0;
}
