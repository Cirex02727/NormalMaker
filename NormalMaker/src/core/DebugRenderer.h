#pragma once

struct DebugRendererVertex
{
	glm::vec2 Position;
	glm::vec3 Color;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription()
	{
		return std::vector<VkVertexInputBindingDescription>
		{
			VkVertexInputBindingDescription
			{
				/* binding   */ 0,
				/* stride    */ sizeof(DebugRendererVertex),
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
				/* offset   */ offsetof(DebugRendererVertex, Position)
			},
			VkVertexInputAttributeDescription
			{
				/* location */ 1,
				/* binding  */ 0,
				/* format   */ VK_FORMAT_R32G32B32_SFLOAT,
				/* offset   */ offsetof(DebugRendererVertex, Color)
			}
		};
	}
};

class DebugRenderer
{
public:
	DebugRenderer(VkDevice device, VkPhysicalDevice physicalDevice, MappedBuffer uniformBuffer, VkSampleCountFlagBits msaaSamples,
		VkRenderPass renderPass, uint32_t bufferCapacity = 1024, float lineWidth = 1.0f);

	void AddLines(const std::vector<DebugRendererVertex>& lines);

	void ClearLines();

	void Render(VkCommandBuffer commandBuffer, float alpha);

	void Delete(VkDevice device);

private:
	void CreateDescriptorPool(VkDevice device);
	void CreateDescriptorSetLayout(VkDevice device);
	void CreateDescriptorSet(VkDevice device, MappedBuffer uniformBuffer);

private:
	VkDescriptorPool      m_DescriptorPool      = nullptr;
	VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;
	VkDescriptorSet       m_DescriptorSet       = nullptr;

	VkPipelineLayout      m_PipelineLayout      = nullptr;
	VkPipeline            m_Pipeline            = nullptr;

	MappedBuffer          m_Buffer              = {};
	uint32_t              m_LinesCount          = 0;

	float m_LineWidth = 1.0f;
};
