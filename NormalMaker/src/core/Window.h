#pragma once

class Window
{
public:
	const enum class WindowState { Windowed, Fullscreen, WindowedBorderless };

public:
	static const char* WindowStateToString(const WindowState state)
	{
		switch (state)
		{
		case WindowState::Windowed:           return "Windowed";
		case WindowState::Fullscreen:         return "Fullscreen";
		case WindowState::WindowedBorderless: return "WindowedBorderless";
		default:                              return "";
		}
	}

public:
	static bool CreateGLFWWindow(int width, int height, const char* title);

	static void SetWindowUserPointer(void* pointer) { glfwSetWindowUserPointer(m_Window, pointer); }

	static void SetFramebufferSizeCallback(GLFWframebuffersizefun callback) { glfwSetFramebufferSizeCallback(m_Window, callback); }

	static void GetFramebufferSize(int* width, int* height) { return glfwGetFramebufferSize(m_Window, width, height); }

	static bool WindowShouldClose() { return glfwWindowShouldClose(m_Window); }

	static void SetWindowState(const WindowState state);

	static void SetWindowWindowed(bool borderless = false);
	static void SetWindowFullscreen();

	static void DestroyWindow();

	static GLFWwindow* GetWindow() { return m_Window; }

	static WindowState GetWindowState() { return m_WindowState; }

	static glm::ivec2 GetWindowPos()
	{
		glm::ivec2 pos{ -1.0f, -1.0f };
		glfwGetWindowPos(m_Window, &pos.x, &pos.y);
		return pos;
	}

private:
	static GLFWmonitor* GetCurrentMonitor();

private:
	static GLFWwindow* m_Window;

	static int m_PrevWidth;
	static int m_PrevHeight;
	static int m_PrevX;
	static int m_PrevY;

	static WindowState m_WindowState;
};
