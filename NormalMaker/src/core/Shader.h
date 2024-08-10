#pragma once

class Shader
{
public:
	static void CreateGraphicsPipeline(VkDevice device, const std::string& vertexShader, const std::string& fragmentShader,
		std::vector<VkVertexInputBindingDescription> bindingDescriptions, std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
		const glm::uvec2& viewportSize, VkSampleCountFlagBits m_MSAASamples, VkBool32 depthTest,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants,
		VkRenderPass renderPass, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, float lineWidth = 1.0f);

	static void CreateComputePipeline(VkDevice device, const std::string& computeShader,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants,
		VkPipelineLayout& pipelineLayout, VkPipeline& pipeline);

private:
	static size_t ReadShaderFile(const std::string& filename, unsigned int** code);

	static VkShaderModule CreateShaderModule(VkDevice device, const unsigned int* data, size_t size);
};
