#pragma once

struct Layer
{
	std::unique_ptr<Texture> Texture;

	VkDescriptorSet DescriptorSet;

	glm::ivec2 Position = {};
	float      ZOff     = 0.0f;

	std::string Name = "-NULL-";

	float Alpha = 1.0f;

	bool IsNormal = false;
};

struct LayerVertex
{
	glm::vec2 Position;
	glm::vec2 UV;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription()
	{
		return std::vector<VkVertexInputBindingDescription>
		{
			VkVertexInputBindingDescription
			{
				/* binding   */ 0,
				/* stride    */ sizeof(LayerVertex),
				/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
			}
		};
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		return std::vector<VkVertexInputAttributeDescription>
		{
			VkVertexInputAttributeDescription
			{
				/* location */ 0,
				/* binding  */ 0,
				/* format   */ VK_FORMAT_R32G32_SFLOAT,
				/* offset   */ offsetof(LayerVertex, Position)
			},
				VkVertexInputAttributeDescription
			{
				/* location */ 1,
				/* binding  */ 0,
				/* format   */ VK_FORMAT_R32G32_SFLOAT,
				/* offset   */ offsetof(LayerVertex, UV)
			}
		};
	}
};

struct PushConstants
{
	glm::ivec2 Position   = {};
	glm::ivec2 Size       = {};
	glm::ivec2 CanvasSize = {};

	float      ZOff       = 0.0f;
	float      Padding    = 0.0f;
	float      Alpha      = 0.0f;
};

struct PaintConstants
{
	glm::vec4  Color;
	glm::ivec2 ImageSize;
	glm::ivec2 Position;
	int        Radius;
};

struct CombineConstants
{
	glm::ivec2 ImageSize;
	glm::ivec2 Position;
};

class LayerManager
{
public:
	static const int MAX_LAYERS = 16;

public:
	LayerManager(Application* application, MappedBuffer& uniformBuffer);

	void Delete();

	void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, const glm::ivec2& canvasSize) const;

	bool AddLayerFromFile(const std::string& filepath, glm::ivec2& canvasSize);

	void AddNormalLayer(int width, int height);

	void ClearLayers(VkDevice device);

	void PaintLayer(uint32_t layerId, const glm::ivec2& imageSize, const glm::ivec2& position, int radius, const glm::vec4& color);

	void ClearCurrPaintLayer(VkDevice device);

	void CombineLayers(const std::string& filepath, const glm::ivec2& canvasSize);

	void SaveLayers(std::ofstream& out);

	void LoadLayers(std::ifstream& in);

	std::vector<Layer>& GetLayers() { return m_Layers; }

private:
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();

	void CreateDescriptorSet(Layer& layer);

	void CreatePaintDescriptorSetLayout();

	void CreatePaintDescriptorSet(Layer& layer);

	void CreateCombineDescriptorSetLayout();

	void CreateCombineDescriptorSet(const Texture& outTexture, const Layer& layer);

	inline float GetMaxZOff() const
	{
		float max = m_Layers.size() > 0 ? m_Layers[0].ZOff : 0.0f;
		for (const Layer& layer : m_Layers)
			max = std::max(max, layer.ZOff + 1.0f);

		return max;
	}

private:
	Application*              m_Application    = nullptr;

	MappedBuffer              m_UniformBuffer = {};

	VkDescriptorPool             m_DescriptorPool      = nullptr;
	VkDescriptorSetLayout        m_DescriptorSetLayout = nullptr;

	BufferData m_LayerVertexBuffer = {};

	std::vector<Layer> m_Layers = {};

	VkPipelineLayout m_PipelineLayout = nullptr;
	VkPipeline       m_Pipeline       = nullptr;

	uint32_t              m_CurrentPaintLayer        = -1;

	// ----------------------- Paint ----------------------- //
	VkDescriptorSetLayout m_PaintDescriptorSetLayout = nullptr;
	VkDescriptorSet       m_PaintDescriptorSet       = nullptr;

	VkPipelineLayout      m_PaintPipelineLayout      = nullptr;
	VkPipeline            m_PaintPipeline            = nullptr;

	// ----------------------- Combine ----------------------- //
	VkDescriptorSetLayout m_CombineDescriptorSetLayout = nullptr;
	VkDescriptorSet       m_CombineDescriptorSet       = nullptr;

	VkPipelineLayout      m_CombinePipelineLayout      = nullptr;
	VkPipeline            m_CombinePipeline            = nullptr;
};
