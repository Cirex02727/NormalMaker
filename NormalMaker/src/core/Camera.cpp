#include "vkpch.h"
#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "input/Input.h"

bool Camera::OnUpdate(float ts)
{
	if (!m_Application->IsViewportHovered())
		return false;

	bool moved = false;

	constexpr glm::vec3 upDirection   (0.0f, 1.0f, 0.0f);
	constexpr glm::vec3 rightDirection(1.0f, 0.0f, 0.0f);

	float speed = 2.5f * m_Zoom;

	// Movement
	if (Input::IsKey(KeyCode::W))
	{
		m_Position += upDirection * speed * ts;
		moved = true;
	}
	else if (Input::IsKey(KeyCode::S))
	{
		m_Position -= upDirection * speed * ts;
		moved = true;
	}
	if (Input::IsKey(KeyCode::A))
	{
		m_Position += rightDirection * speed * ts;
		moved = true;
	}
	else if (Input::IsKey(KeyCode::D))
	{
		m_Position -= rightDirection * speed * ts;
		moved = true;
	}

	float scrollSpeed = 10.0f;
	float scroll = Input::GetMouseScroll().y * scrollSpeed;
	if (scroll != 0.0f)
	{
		m_Zoom = std::clamp(m_Zoom - scroll, 0.1f, 1000.0f);

		RecalculateOrtho();
	}

	if (moved)
		RecalculateView();

	return moved;
}

void Camera::OnResize(uint32_t width, uint32_t height)
{
	float aspect = (float)width / height;
	if (width == 0 || height == 0 || aspect == m_AspectRatio)
		return;

	m_AspectRatio = aspect;

	RecalculateOrtho();
}

void Camera::RecalculateOrtho()
{
	m_Ortho = glm::ortho(-m_AspectRatio * m_Zoom, m_AspectRatio * m_Zoom, -m_Zoom, m_Zoom, -10.0f, 10.0f);
}

void Camera::RecalculateView()
{
	m_View = glm::translate(glm::mat4(1.0f), m_Position);
}

void Camera::SaveSettings() const
{
	YAML::Node node = SaveManager::GetNode("Camera");
	node["Position"] = m_Position;
	node["Zoom"] = m_Zoom;
}

void Camera::LoadSettings()
{
	YAML::Node node = SaveManager::GetNode("Camera");
	if (node["Position"].IsDefined())
		m_Position = node["Position"].as<glm::vec3>();

	if (node["Zoom"].IsDefined())
		m_Zoom = node["Zoom"].as<float>();

	RecalculateOrtho();
	RecalculateView();
}
