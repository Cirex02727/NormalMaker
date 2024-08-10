#pragma once

class Application;

class ApplicationLayer
{
public:
	virtual void Init(Application& application) {}
	virtual void Destroy() {}

	virtual void OnUpdate(float dt) {}

	virtual void OnImGuiRenderMenuBar(bool& isRunning) {}
	virtual void OnImGuiRender() {}

	virtual void OnPreRender(VkCommandBuffer commandBuffer, uint32_t imageIndex) {}
	virtual void OnRender(VkCommandBuffer commandBuffer, uint32_t imageIndex) {}
	virtual void OnPostRender(VkCommandBuffer commandBuffer, uint32_t imageIndex) {}

	virtual void OnResize(const glm::uvec2& size) {}
};
