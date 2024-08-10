#pragma once

#include "KeyCodes.h"

struct GLFWwindow;

class Input
{
public:
	static void Init();
	static void Update();

	static void SetCursorMode(CursorMode mode);

	static bool IsKey(KeyCode keycode);
	static bool IsKeyDown(KeyCode keycode);
	static bool IsKeyUp(KeyCode keycode);

	static bool IsMouse(MouseButton mouseButton);
	static bool IsMouseDown(MouseButton mouseButton);
	static bool IsMouseUp(MouseButton mouseButton);

	static const glm::vec2& GetMousePosition() { return m_CurrCursorPos; }
	static const glm::vec2& GetMouseDelta()    { return m_CursorDelta;   }

	static const glm::vec2& GetMouseScroll()   { return m_MouseScroll;   }

private:
	static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void OnMouse(GLFWwindow* window, int button, int action, int mods);
	static void OnCursorPos(GLFWwindow* window, double xpos, double ypos);
	static void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);

private:
	static GLFWwindow* m_WindowHandle;

	static bool m_PrevKeys[(unsigned long long)KeyCode::Count];
	static bool m_CurrKeys[(unsigned long long)KeyCode::Count];

	static bool m_PrevMouse[(unsigned long long)MouseButton::Count];
	static bool m_CurrMouse[(unsigned long long)MouseButton::Count];

	static glm::vec2 m_PrevCursorPos;
	static glm::vec2 m_CurrCursorPos;
	static glm::vec2 m_CursorDelta;

	static glm::vec2 m_MouseScroll;
};
