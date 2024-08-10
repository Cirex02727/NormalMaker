#pragma once

#include <glm/glm.hpp>
#include <vector>

class Camera
{
public:
	Camera(Application* application)
		: m_Application(application) {}

	bool OnUpdate(float ts);
	void OnResize(uint32_t width, uint32_t height);

	void RecalculateView();

	void SaveSettings() const;
	void LoadSettings();

	void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateView(); }
	void SetZoom(float zoom) { m_Zoom = zoom; RecalculateOrtho(); }

	const glm::mat4& GetOrtho()     const { return m_Ortho;    }
	const glm::mat4& GetView()      const { return m_View;     }

	const glm::vec3& GetPosition()  const { return m_Position; }

	glm::vec3& GetPositionR()             { return m_Position; }

	float GetZoom() const { return m_Zoom; }

	glm::vec2 GetOrthoExtension() const { return { m_AspectRatio * m_Zoom, -m_Zoom }; }

private:
	void RecalculateOrtho();

private:
	Application* m_Application = nullptr;

	glm::mat4 m_Ortho{ 1.0f };
	glm::mat4 m_View { 1.0f };

	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };

	float m_AspectRatio = 0.0f;
	float m_Zoom        = 1.0f;
};
