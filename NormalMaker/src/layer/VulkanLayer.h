#include "data/LayerManager.h"

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 ortho;
};

struct NormalArrow
{
	glm::vec2 Start = {};
	glm::vec2 End   = {};

	float     Angle = 0.001f;

	float     Padding[3]{};
};

struct NormalArrows
{
	static const int MAX_ARROWS = 256;

	NormalArrow Arrows[MAX_ARROWS]{};
	int         Count = 0;
};

class VulkanLayer : public ApplicationLayer
{
public:
	virtual void Init(Application& application);

private:
	void CreateUniformBuffer();
	void UpdateUniformBuffer() const;

	void CreateNormalArrowsUniformBuffer();

	void CreateDescriptorPool();

	void CreateNormalDescriptorSetLayout();
	void CreateNormalDescriptorSet(const Layer& layer);

public:
	virtual void Destroy();

	virtual void OnUpdate(float dt);

	virtual void OnImGuiRenderMenuBar(bool& isRunning);
	virtual void OnImGuiRender();

	virtual void OnPreRender(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	virtual void OnRender(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	virtual void OnPostRender(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	virtual void OnResize(const glm::uvec2& size);

private:
	void DrawNormalArrows();
	void CalculateNormalArrow(std::vector<DebugRendererVertex>& lines, const NormalArrow& arrow, const glm::vec3& color) const;

	void DispatchNormal(const Layer& layer) const;

	void ClearProject(bool clearLayers = true);

	void SaveProject();
	void LoadProject();

	void BuildGrid();

private:
	glm::vec2 GetMouseWorldPosition() const;

private:
	// ------------------------- Device ------------------------ //
	Application*          m_Application          = {};

	VkDevice              m_Device               = nullptr;
	VkPhysicalDevice      m_PhysicalDevice       = nullptr;
	VkQueue               m_Queue                = nullptr;
	VkCommandPool         m_CommandPool          = nullptr;

	VkRenderPass          m_RenderPass           = nullptr;

	VkFormat              m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

	VkSampleCountFlagBits m_MSAASamples          = VK_SAMPLE_COUNT_1_BIT;

	VkImageView           m_DepthImageView       = nullptr;

	VkSampler             m_ViewportImageSampler = nullptr;

	uint32_t              m_ImageCount           = 0;

	VkDescriptorPool      m_ImGuiDescriptorPool  = nullptr;

	glm::uvec2            m_ViewportSize         = {};

	// ---------------------- Global --------------------- //
	std::shared_ptr<Camera>       m_Camera;
	glm::uvec2                    m_PrevViewportRegionAvail = {};

	MappedBuffer                  m_UniformBuffer           = {};

	std::unique_ptr<LayerManager> m_LayerManager;

	VkDescriptorPool              m_DescriptorPool          = nullptr;

	// ------------------ Debug Lines ----------------- //
	std::unique_ptr<DebugRenderer> m_GridRenderer;
	std::unique_ptr<DebugRenderer> m_NormalArrowsRenderer;


	std::string m_CurrProject     = "";
	bool        m_IsProjectLoaded = false;

	glm::ivec2 m_CanvasSize = {};

	float     m_GridDepth      = 75.0f;

	bool      m_UseNormalBrush = true;
	bool      m_UseEraser      = false;

	glm::vec4 m_BrushColor     = { 1.0f, 1.0f, 1.0f, 1.0f };
	int       m_BrushRadius    = 1;

	int       m_SelectedLayer  = -1;
	
	// -------------------- Normal Arrows -------------------- //
	NormalArrows          m_NormalArrows              = {};
	MappedBuffer          m_NormalArrowsUniformBuffer = {};

	bool                  m_IsMovingNormalArrow       = false;
	int                   m_SelectedNormalArrow       = -1;

	VkDescriptorSetLayout m_NormalDescriptorSetLayout = nullptr;
	VkDescriptorSet       m_NormalDescriptorSet       = nullptr;

	VkPipelineLayout      m_NormalPipelineLayout      = nullptr;
	VkPipeline            m_NormalPipeline            = nullptr;
};
