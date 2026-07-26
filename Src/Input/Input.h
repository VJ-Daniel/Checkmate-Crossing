#pragma once

#include <glm.hpp>
#include <glfw3.h>

enum class Key
{
    Unknown = GLFW_KEY_UNKNOWN,

    Space = GLFW_KEY_SPACE,

    A = GLFW_KEY_A,
    B = GLFW_KEY_B,
    C = GLFW_KEY_C,
    D = GLFW_KEY_D,
    E = GLFW_KEY_E,
    F = GLFW_KEY_F,
    G = GLFW_KEY_G,
    H = GLFW_KEY_H,
    I = GLFW_KEY_I,
    J = GLFW_KEY_J,
    K = GLFW_KEY_K,
    L = GLFW_KEY_L,
    M = GLFW_KEY_M,
    N = GLFW_KEY_N,
    O = GLFW_KEY_O,
    P = GLFW_KEY_P,
    Q = GLFW_KEY_Q,
    R = GLFW_KEY_R,
    S = GLFW_KEY_S,
    T = GLFW_KEY_T,
    U = GLFW_KEY_U,
    V = GLFW_KEY_V,
    W = GLFW_KEY_W,
    X = GLFW_KEY_X,
    Y = GLFW_KEY_Y,
    Z = GLFW_KEY_Z,

    Escape = GLFW_KEY_ESCAPE,

    Enter = GLFW_KEY_ENTER,

    Left = GLFW_KEY_LEFT,
    Right = GLFW_KEY_RIGHT,
    Up = GLFW_KEY_UP,
    Down = GLFW_KEY_DOWN,

    LeftShift = GLFW_KEY_LEFT_SHIFT,
    LeftControl = GLFW_KEY_LEFT_CONTROL,
    LeftAlt = GLFW_KEY_LEFT_ALT,

    RightShift = GLFW_KEY_RIGHT_SHIFT,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    RightAlt = GLFW_KEY_RIGHT_ALT
};

enum class MouseButton
{
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE
};

enum class GamepadButton
{
    A = GLFW_GAMEPAD_BUTTON_A,
    B = GLFW_GAMEPAD_BUTTON_B,
    X = GLFW_GAMEPAD_BUTTON_X,
    Y = GLFW_GAMEPAD_BUTTON_Y,
    LeftBumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
    RightBumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
    Back = GLFW_GAMEPAD_BUTTON_BACK,
    Start = GLFW_GAMEPAD_BUTTON_START,
    DPadRight = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
    DPadLeft = GLFW_GAMEPAD_BUTTON_DPAD_LEFT
};

/// Polling facade that unifies keyboard, mouse and GLFW gamepad state.
class Input
{
public:

    static void Initialize(GLFWwindow* window);

    /// Polls window events and caches the first recognized gamepad state.
    static void Update();

    static bool IsKeyPressed(Key key);

    static bool IsMouseButtonPressed(MouseButton button);

    static bool IsGamepadConnected();

    static bool IsGamepadButtonPressed(GamepadButton button);

    static glm::vec2 GetGamepadLeftStick();

    static glm::vec2 GetMousePosition();

    static GLFWwindow* GetWindow();

private:

    static GLFWwindow* window;

    static bool gamepadConnected;

    static GLFWgamepadstate gamepadState;
};
