#include "vkpch.h"
#include "Window.h"

GLFWwindow* Window::m_Window = nullptr;

int Window::m_PrevWidth = 0;
int Window::m_PrevHeight = 0;
int Window::m_PrevX = 0;
int Window::m_PrevY = 0;

Window::WindowState Window::m_WindowState = Window::WindowState::Windowed;

bool Window::CreateGLFWWindow(int width, int height, const char* title)
{
    m_PrevWidth = width;
    m_PrevHeight = height;

    YAML::Node node = SaveManager::GetNode("Window");
    if (node["PrevWidth"].IsDefined())  m_PrevWidth  = node["PrevWidth"].as<int>();
    if (node["PrevHeight"].IsDefined()) m_PrevHeight = node["PrevHeight"].as<int>();
    
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Window = glfwCreateWindow(m_PrevWidth, m_PrevHeight, title, nullptr, nullptr);
    if (!m_Window)
    {
        printf("Error create Window\n");
        glfwTerminate();
        return false;
    }

    if (!glfwVulkanSupported())
    {
        printf("Error Vulkan is not supported!");
        glfwTerminate();
        return false;
    }


    if (node["PrevX"].IsDefined() && node["PrevY"].IsDefined())
    {
        m_PrevX = node["PrevX"].as<int>();
        m_PrevY = node["PrevY"].as<int>();
    }
    else
        glfwGetWindowPos(m_Window, &m_PrevX, &m_PrevY);

    if (node["WindowState"].IsDefined())
    {
        m_WindowState = WindowState::WindowedBorderless;
        SetWindowState((WindowState)node["WindowState"].as<int>());
    }

    return true;
}

void Window::SetWindowState(const WindowState state)
{
    switch (state)
    {
    case Window::WindowState::Windowed:
        SetWindowWindowed();
        break;
    case Window::WindowState::Fullscreen:
        SetWindowFullscreen();
        break;
    case Window::WindowState::WindowedBorderless:
        SetWindowWindowed(true);
        break;
    }
}

void Window::SetWindowWindowed(bool borderless)
{
    if (m_WindowState == WindowState::Windowed)
    {
        glfwGetWindowSize(m_Window, &m_PrevWidth, &m_PrevHeight);
        glfwGetWindowPos(m_Window, &m_PrevX, &m_PrevY);
    }

    m_WindowState = borderless ? Window::WindowState::WindowedBorderless : Window::WindowState::Windowed;

    glfwSetWindowAttrib(m_Window, GLFW_DECORATED, !borderless);

    GLFWmonitor* monitor = GetCurrentMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

    int mX = 0, mY = 0;
    glfwGetMonitorPos(monitor, &mX, &mY);

    if(borderless)
        glfwSetWindowMonitor(m_Window, nullptr, mX, mY, videoMode->width, videoMode->height, GLFW_DONT_CARE);
    else
        glfwSetWindowMonitor(m_Window, nullptr, m_PrevX, m_PrevY, m_PrevWidth, m_PrevHeight, GLFW_DONT_CARE);
}

void Window::SetWindowFullscreen()
{
    if (m_WindowState == WindowState::Windowed)
    {
        glfwGetWindowSize(m_Window, &m_PrevWidth, &m_PrevHeight);
        glfwGetWindowPos(m_Window, &m_PrevX, &m_PrevY);
    }

    m_WindowState = Window::WindowState::Fullscreen;

    GLFWmonitor* monitor = GetCurrentMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

    int mX = 0, mY = 0;
    glfwGetMonitorPos(monitor, &mX, &mY);

    glfwSetWindowMonitor(m_Window, monitor, mX, mY, videoMode->width, videoMode->height, GLFW_DONT_CARE);
}

void Window::DestroyWindow()
{
    if (m_WindowState == WindowState::Windowed)
    {
        glfwGetWindowSize(m_Window, &m_PrevWidth, &m_PrevHeight);
        glfwGetWindowPos(m_Window, &m_PrevX, &m_PrevY);
    }

    YAML::Node node = SaveManager::GetNode("Window");
    node["WindowState"] = (int)m_WindowState;
    node["PrevWidth"] = m_PrevWidth;
    node["PrevHeight"] = m_PrevHeight;
    node["PrevX"] = m_PrevX;
    node["PrevY"] = m_PrevY;

    glfwDestroyWindow(m_Window);
}

GLFWmonitor* Window::GetCurrentMonitor()
{
    int wX = 0, wY = 0;
    glfwGetWindowPos(m_Window, &wX, &wY);

    int wW = 0, wH = 0;
    glfwGetWindowSize(m_Window, &wW, &wH);

    int midX = (int)(wX + wW * 0.5f), midY = (int)(wY + wH * 0.5f);

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    for (int i = 0; i < count; ++i)
    {
        GLFWmonitor* monitor = monitors[i];

        int mX = 0, mY = 0;
        glfwGetMonitorPos(monitor, &mX, &mY);

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (midX < mX || midX > mX + mode->width ||
            midY < mY || midY > mY + mode->height)
            continue;

        return monitor;
    }

    return nullptr;
}
