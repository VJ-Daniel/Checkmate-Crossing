/*
    ============================================================
    Checkmate Crossing - OpenGL

    Description:

    A 3D arcade obstacle-crossing game with a medieval chess theme.
    The player guides a pawn across a battlefield of dangerous lanes
    to rescue the captured king.

    This build is the project foundation: it stands up the window, the
    rendering pipeline, the battlefield and the player placeholder, so
    gameplay can be layered on top without restructuring anything.

    Features in this build:

        - OpenGL and GLFW window initialization
        - GLEW function loading
        - Tilted orthographic 3D camera, ready to follow the player
        - Indexed 3D meshes drawn from VAO / VBO / EBO
        - Directional and ambient lighting
        - Lane-based battlefield built from the level design
        - Placeholder pawn at the starting position

    Technologies:

        - OpenGL 3.3 Core Profile
        - GLFW
        - GLEW
        - GLM
        - stb_image
        - Windows Multimedia API
        - C++

    Based on the Gangster Survival OpenGL framework by Leonardo Moura.
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

        // Goes through the game rather than straight to the camera, so the
        // screen-space sprite projection is resized along with it.
        game.SetViewportSize(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    bool InitializeWindow()
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW.\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        window = glfwCreateWindow(
            ScreenWidth, ScreenHeight, "Checkmate Crossing", nullptr, nullptr);

        if (!window)
        {
            std::cerr << "Failed to create the window.\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "Failed to initialize GLEW.\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }

        // Clear any error that GLEW might have generated
        glGetError();

        glViewport(0, 0, ScreenWidth, ScreenHeight);

        // Required now that the world is 3D: without it, whichever object
        // was drawn last would cover the ones in front of it.
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Print OpenGL info for debugging
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "Window created successfully: " << ScreenWidth << "x" << ScreenHeight << std::endl;

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
        std::cerr << "Failed to initialize the game.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Game initialized. Starting main loop..." << std::endl;

    double previousTime = glfwGetTime();
    int frameCount = 0;
    bool windowVisible = true;

    while (!glfwWindowShouldClose(window))
    {
        // Process window events - CRITICAL for window responsiveness
        glfwPollEvents();

        Input::Update();

        if (Input::IsKeyPressed(Key::Escape) ||
            Input::IsGamepadButtonPressed(GamepadButton::Start))
        {
            std::cout << "Escape pressed, closing window..." << std::endl;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(
            std::min(currentTime - previousTime, 0.05));
        previousTime = currentTime;

        game.Update(deltaTime);

        glClearColor(0.11f, 0.13f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        game.Render();

        glfwSwapBuffers(window);

        // Debug: print frame count every 60 frames
        frameCount++;
        if (frameCount % 60 == 0)
        {
            std::cout << "Frame " << frameCount << " - Delta: " << deltaTime << "s" << std::endl;
        }

        // Check if window is still visible
        if (frameCount == 10)
        {
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            if (windowWidth == 0 || windowHeight == 0)
            {
                std::cout << "Warning: Window size is 0x0 - window might be minimized!" << std::endl;
                // Restore the window if it's minimized
                glfwRestoreWindow(window);
            }
        }
    }

    std::cout << "Main loop ended after " << frameCount << " frames." << std::endl;

    game.Shutdown();
    glfwDestroyWindow(window);
    window = nullptr;
    glfwTerminate();

    std::cout << "Game exited." << std::endl;
    return 0;
}