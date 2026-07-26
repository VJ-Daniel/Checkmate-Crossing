/*
    ============================================================
    Gangster Survival - Input
    Author: Leonardo Moura | Date: 2026

    Polls GLFW events and provides one interface for keyboard, mouse,
    gamepad buttons and analog stick input with a dead zone.
    ============================================================
*/

#include "Input.h"

GLFWwindow* Input::window = nullptr;
bool Input::gamepadConnected = false;
GLFWgamepadstate Input::gamepadState = {};

void Input::Initialize(GLFWwindow* window)
{
    Input::window = window;
}

void Input::Update()
{
    glfwPollEvents();

    gamepadConnected = false;

    for (int joystick = GLFW_JOYSTICK_1;
        joystick <= GLFW_JOYSTICK_LAST; ++joystick)
    {
        if (glfwJoystickIsGamepad(joystick) &&
            glfwGetGamepadState(joystick, &gamepadState))
        {
            gamepadConnected = true;
            break;
        }
    }

    if (!gamepadConnected)
        gamepadState = {};
}

bool Input::IsKeyPressed(Key key)
{
    if (window == nullptr)
    {
        return false;
    }

    return glfwGetKey(
        window,
        static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(MouseButton button)
{
    if (window == nullptr)
    {
        return false;
    }

    return glfwGetMouseButton(
        window,
        static_cast<int>(button)) == GLFW_PRESS;
}

bool Input::IsGamepadConnected()
{
    return gamepadConnected;
}

bool Input::IsGamepadButtonPressed(GamepadButton button)
{
    if (!gamepadConnected)
        return false;

    return gamepadState.buttons[static_cast<int>(button)] == GLFW_PRESS;
}

glm::vec2 Input::GetGamepadLeftStick()
{
    if (!gamepadConnected)
        return glm::vec2(0.0f);

    glm::vec2 stick(
        gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X],
        gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);

    constexpr float deadZone = 0.25f;
    if (glm::abs(stick.x) < deadZone)
        stick.x = 0.0f;
    if (glm::abs(stick.y) < deadZone)
        stick.y = 0.0f;

    return stick;
}

glm::vec2 Input::GetMousePosition()
{
    if (window == nullptr)
    {
        return glm::vec2(0.0f);
    }

    double x;
    double y;

    glfwGetCursorPos(
        window,
        &x,
        &y);

    return glm::vec2(
        static_cast<float>(x),
        static_cast<float>(y));
}

GLFWwindow* Input::GetWindow()
{
    return window;
}
