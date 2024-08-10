#include "vkpch.h"
#include "DebugRenderer.h"

#include "layer/VulkanLayer.h"

DebugRenderer::DebugRenderer(VkDevice device, VkPhysicalDevice physicalDevice, MappedBuffer uniformBuffer,
    VkSampleCountFlagBits msaaSamples, VkRenderPass renderPass, uint32_t bufferCapacity, float lineWidth)
    : m_LineWidth(lineWidth)
{
    CreateDescriptorPool(device);
    CreateDescriptorSetLayout(device);
    CreateDescriptorSet(device, uniformBuffer);

    VkPushConstantRange pushConstants
    {
        /* stageFlags */ VK_SHADER_STAGE_FRAGMENT_BIT,
        /* offset     */ 0,
        /* size       */ sizeof(float)
    };

    Shader::CreateGraphicsPipeline(device, "debuglines.vert", "debuglines.frag", { DebugRendererVertex::getBindingDescription() },
        { DebugRendererVertex::getAttributeDescriptions() }, {}, msaaSamples, VK_FALSE,
        { m_DescriptorSetLayout }, { pushConstants }, renderPass, m_PipelineLayout, m_Pipeline,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST, lineWidth);

    m_Buffer = Buffer::CreateMappedBuffer(device, physicalDevice, static_cast<uint32_t>(sizeof(DebugRendererVertex) * bufferCapacity),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void DebugRenderer::AddLines(const std::vector<DebugRendererVertex>& lines)
{
    memcpy((DebugRendererVertex*)m_Buffer.Map + m_LinesCount, lines.data(), static_cast<uint32_t>(sizeof(DebugRendererVertex) * lines.size()));
    m_LinesCount += static_cast<uint32_t>(lines.size());
}

void DebugRenderer::ClearLines()
{
    m_LinesCount = 0;
}

void DebugRenderer::Render(VkCommandBuffer commandBuffer, float alpha)
{
    if (m_LinesCount <= 0)
        return;

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Buffer.Buffer, offsets);

    vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &alpha);

    if(m_LineWidth != 1.0f)
        vkCmdSetLineWidth(commandBuffer, m_LineWidth);

    vkCmdDraw(commandBuffer, m_LinesCount, 1, 0, 0);

    vkCmdSetLineWidth(commandBuffer, 1.0f);
}

void DebugRenderer::Delete(VkDevice device)
{
    DeleteBuffer(device, m_Buffer);

    vkDestroyPipeline(device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
}

void DebugRenderer::CreateDescriptorPool(VkDevice device)
{
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount */ 1
        }
    };

    VkDescriptorPoolCreateInfo poolInfo
    {
        /* sType         */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext         */ nullptr,
        /* flags         */ 0,
        /* maxSets       */ 1,
        /* poolSizeCount */ static_cast<uint32_t>(poolSizes.size()),
        /* pPoolSizes    */ poolSizes.data()
    };

    VK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool));
}

void DebugRenderer::CreateDescriptorSetLayout(VkDevice device)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
    {
        VkDescriptorSetLayoutBinding
        {
            /* binding            */ 0,
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount    */ 1,
            /* stageFlags         */ VK_SHADER_STAGE_VERTEX_BIT,
            /* pImmutableSamplers */ nullptr
        }
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo
    {
        /* sType        */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        /* pNext        */ nullptr,
        /* flags        */ 0,
        /* bindingCount */ static_cast<uint32_t>(layoutBindings.size()),
        /* pBindings    */ layoutBindings.data()
    };

    VK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout));
}

void DebugRenderer::CreateDescriptorSet(VkDevice device, MappedBuffer uniformBuffer)
{
    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_DescriptorPool,
        /* descriptorSetCount */ 1,
        /* pSetLayouts        */ &m_DescriptorSetLayout
    };
    VK(vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet));


    VkDescriptorBufferInfo bufferInfo
    {
        /* buffer */ uniformBuffer.Buffer,
        /* offset */ 0,
        /* range  */ sizeof(UniformBufferObject)
    };

    VkWriteDescriptorSet descriptorSet
    {
        /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        /* pNext            */ nullptr,
        /* dstSet           */ m_DescriptorSet,
        /* dstBinding       */ 0,
        /* dstArrayElement  */ 0,
        /* descriptorCount  */ 1,
        /* descriptorType   */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        /* pImageInfo       */ nullptr,
        /* pBufferInfo      */ &bufferInfo,
        /* pTexelBufferView */ nullptr
    };

    vkUpdateDescriptorSets(device, 1, &descriptorSet, 0, nullptr);
}
