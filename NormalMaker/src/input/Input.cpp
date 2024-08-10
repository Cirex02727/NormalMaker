#include "vkpch.h"
#include "Input.h"

GLFWwindow* Input::m_WindowHandle = nullptr;

glm::vec2 Input::m_MouseScroll = {};

bool Input::m_PrevKeys[(unsigned long long)KeyCode::Count];
bool Input::m_CurrKeys[(unsigned long long)KeyCode::Count];

bool Input::m_PrevMouse[(unsigned long long)MouseButton::Count];
bool Input::m_CurrMouse[(unsigned long long)MouseButton::Count];

glm::vec2 Input::m_PrevCursorPos = {};
glm::vec2 Input::m_CurrCursorPos = {};

glm::vec2 Input::m_CursorDelta = {};

void Input::Init()
{
	m_WindowHandle = Window::GetWindow();

	memset(m_PrevKeys, false, (unsigned long long)KeyCode::Count * sizeof(bool));
	memset(m_CurrKeys, false, (unsigned long long)KeyCode::Count * sizeof(bool));

	memset(m_PrevMouse, false, (unsigned long long)MouseButton::Count * sizeof(bool));
	memset(m_CurrMouse, false, (unsigned long long)MouseButton::Count * sizeof(bool));

	glfwSetKeyCallback        (m_WindowHandle, OnKey        );
	glfwSetMouseButtonCallback(m_WindowHandle, OnMouse      );
	glfwSetCursorPosCallback  (m_WindowHandle, OnCursorPos  );
	glfwSetScrollCallback     (m_WindowHandle, OnMouseScroll);
}

void Input::Update()
{
	m_MouseScroll = {};

	m_CursorDelta = m_CurrCursorPos - m_PrevCursorPos;
	m_PrevCursorPos = m_CurrCursorPos;

	memcpy(m_PrevMouse, m_CurrMouse, (int)MouseButton::Count * sizeof(bool));
}

void Input::SetCursorMode(CursorMode mode)
{
	glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
}

bool Input::IsKey(KeyCode keycode)
{
	return m_CurrKeys[(int)keycode];
}

bool Input::IsKeyDown(KeyCode keycode)
{
	return !m_PrevKeys[(int)keycode] && m_CurrKeys[(int)keycode];
}

bool Input::IsKeyUp(KeyCode keycode)
{
	return m_PrevKeys[(int)keycode] && !m_CurrKeys[(int)keycode];
}

bool Input::IsMouse(MouseButton mouseButton)
{
	return m_CurrMouse[(int)mouseButton];
}

bool Input::IsMouseDown(MouseButton mouseButton)
{
	return !m_PrevMouse[(int)mouseButton] && m_CurrMouse[(int)mouseButton];
}

bool Input::IsMouseUp(MouseButton mouseButton)
{
	return m_PrevMouse[(int)mouseButton] && !m_CurrMouse[(int)mouseButton];
}

void Input::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	m_PrevKeys[key] = m_CurrKeys[key];
	m_CurrKeys[key] = action != GLFW_RELEASE;
}

void Input::OnMouse(GLFWwindow* window, int button, int action, int mods)
{
	m_CurrMouse[button] = action != GLFW_RELEASE;
}

void Input::OnCursorPos(GLFWwindow* window, double xpos, double ypos)
{
	m_CurrCursorPos = { xpos, ypos };
}

void Input::OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	m_MouseScroll = { xoffset, yoffset };
}
