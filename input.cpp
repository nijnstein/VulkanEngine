#include "defines.h"

namespace vkengine
{
    namespace input
    {
        void InputManager::init()
        {
            for (int i = 0; i < KEYBOARD_KEY_COUNT; i++) m_keyboardStatus[i] = KeyState::Released;
            for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) m_mouseButtonStatus[i] = KeyState::Released;
        }

        void InputManager::shutdown()
        {

        }

        void InputManager::update(GLFWwindow* window, float deltaTime)
        {
            //mouse position and movement
            const glm::vec2 lastFrameMousePosition = m_mousePosition;

            double mousePosX;
            double mousePosY;
            glfwGetCursorPos(window, &mousePosX, &mousePosY);
            m_mousePosition = glm::vec2(mousePosX, mousePosY);
            m_mouseMovement = m_mousePosition - lastFrameMousePosition;
            

            //mouse buttons
            for (int i = 0; i < MOUSE_BUTTON_COUNT; i++)
            {
                const bool isKeyDown = glfwGetMouseButton(window, mouseButtonCodeToGLFW[i]) == GLFW_PRESS;
                m_mouseButtonStatus[i] = keystateFromBoolAndPreviousState(isKeyDown, m_mouseButtonStatus[i]);
            }

            //keyboard buttons
            for (int i = 0; i < KEYBOARD_KEY_COUNT; i++)
            {
                const bool isKeyDown = glfwGetKey(window, keyCodeToGLFW[i]) == GLFW_PRESS;
                m_keyboardStatus[i] = keystateFromBoolAndPreviousState(isKeyDown, m_keyboardStatus[i]);
            }
        }

        KeyState InputManager::keystateFromBoolAndPreviousState(const bool isKeyDown, const KeyState previousState)
        {
            if (isKeyDown)
            {
                if (previousState == KeyState::Released)
                {
                    return KeyState::Pressed;
                }
                else
                {
                    return KeyState::Held;
                }
            }
            else
            {
                return KeyState::Released;
            }
        }

        glm::vec2 InputManager::getMouseMovement()
        {
            return m_mouseMovement;
        }

        glm::vec2 InputManager::getMousePosition()
        {
            return m_mousePosition;
        }

        KeyState InputManager::getMouseButtonState(const MouseButton button)
        {
            assert((size_t)button < MOUSE_BUTTON_COUNT);
            return m_mouseButtonStatus[(size_t)button];
        }

        KeyState InputManager::getKeyboardKeyState(const KeyboardKey key)
        {
            assert((size_t)key < KEYBOARD_KEY_COUNT);
            return m_keyboardStatus[(size_t)key];
        }

        bool InputManager::isMouseButtonDown(const MouseButton button)
        {
            assert((size_t)button < MOUSE_BUTTON_COUNT);
            return m_mouseButtonStatus[(size_t)button] != KeyState::Released;
        }

        bool InputManager::isKeyboardKeyDown(const KeyboardKey key)
        {
            assert((size_t)key < KEYBOARD_KEY_COUNT);
            return m_keyboardStatus[(size_t)key] != KeyState::Released;
        }
    }
}