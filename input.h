#pragma once

#define KEYBOARD_KEY_COUNT 45
#define MOUSE_BUTTON_COUNT 2

namespace vkengine
{
    namespace input
    {
        enum class KeyboardKey : uint8_t
        {
            keyA = 0,
            keyB = 1,
            keyC = 2,
            keyD = 3,
            keyE = 4,
            keyF = 5,
            keyG = 6,
            keyH = 7,
            keyI = 8,
            keyJ = 9,
            keyK = 10,
            keyL = 11,
            keyM = 12,
            keyN = 13,
            keyO = 14,
            keyP = 15,
            keyQ = 16,
            keyR = 17,
            keyS = 18,
            keyT = 19,
            keyU = 20,
            keyV = 21,
            keyW = 22,
            keyX = 23,
            keyY = 24,
            keyZ = 25,

            key0 = 26,
            key1 = 27,
            key2 = 28,
            key3 = 29,
            key4 = 30,
            key5 = 31,
            key6 = 32,
            key7 = 33,
            key8 = 34,
            key9 = 35,

            keyShiftLeft = 36,
            keySpacebar = 37,
            keyEsc = 38,
            keyLeftAlt = 39,
            keyEnter = 40,

            keyRight = 41,
            keyLeft = 42, 
            keyDown = 43,
            keyUp = 44
        };

        // must match indices from KeyboardKey
        inline int keyCodeToGLFW[KEYBOARD_KEY_COUNT] = {
            GLFW_KEY_A,
            GLFW_KEY_B,
            GLFW_KEY_C,
            GLFW_KEY_D,
            GLFW_KEY_E,
            GLFW_KEY_F,
            GLFW_KEY_G,
            GLFW_KEY_H,
            GLFW_KEY_I,
            GLFW_KEY_J,
            GLFW_KEY_K,
            GLFW_KEY_L,
            GLFW_KEY_M,
            GLFW_KEY_N,
            GLFW_KEY_O,
            GLFW_KEY_P,
            GLFW_KEY_Q,
            GLFW_KEY_R,
            GLFW_KEY_S,
            GLFW_KEY_T,
            GLFW_KEY_U,
            GLFW_KEY_V,
            GLFW_KEY_W,
            GLFW_KEY_X,
            GLFW_KEY_Y,
            GLFW_KEY_Z,

            GLFW_KEY_0,
            GLFW_KEY_1,
            GLFW_KEY_2,
            GLFW_KEY_3,
            GLFW_KEY_4,
            GLFW_KEY_5,
            GLFW_KEY_6,
            GLFW_KEY_7,
            GLFW_KEY_8,
            GLFW_KEY_9,

            GLFW_KEY_LEFT_SHIFT,
            GLFW_KEY_SPACE,
            GLFW_KEY_ESCAPE,
            GLFW_KEY_LEFT_ALT,
            GLFW_KEY_ENTER,

            GLFW_KEY_RIGHT,
            GLFW_KEY_LEFT,
            GLFW_KEY_DOWN,
            GLFW_KEY_UP,
        };

        enum class MouseButton : uint8_t
        {
            left = 0,
            right = 1,
        };

        // must match indices from MouseButton
        inline int mouseButtonCodeToGLFW[MOUSE_BUTTON_COUNT] = {
            GLFW_MOUSE_BUTTON_1,
            GLFW_MOUSE_BUTTON_2,
        };

        enum class KeyState { Pressed, Held, Released };

        class InputManager
        {
        public:
            void init(GLFWwindow * window);
            void shutdown();
            void update(GLFWwindow* window, float deltaTime);

            VEC2 getMouseMovement();
            VEC2 getMousePosition();
            FLOAT getMouseScroll(); 

            KeyState getMouseButtonState(const MouseButton button);
            KeyState getKeyboardKeyState(const KeyboardKey key);
            bool isMouseButtonDown(const MouseButton button);
            bool isKeyboardKeyDown(const KeyboardKey key);

        private:
            static InputManager* instance; 

            float m_scrollOffset = 0; 
            static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

            KeyState m_keyboardStatus[KEYBOARD_KEY_COUNT];
            KeyState m_mouseButtonStatus[MOUSE_BUTTON_COUNT];

            VEC2 m_mousePosition;
            VEC2 m_mouseMovement;
            float m_mouseScroll;

            KeyState keystateFromBoolAndPreviousState(const bool isKeyDown, const KeyState previousState);
        };
    }
}