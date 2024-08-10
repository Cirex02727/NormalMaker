#include "vkpch.h"
#include "Shader.h"

#include "Core.h"

void Shader::CreateGraphicsPipeline(VkDevice device, const std::string& vertexShader, const std::string& fragmentShader,
    std::vector<VkVertexInputBindingDescription> bindingDescriptions, std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
    const glm::uvec2& viewportSize, VkSampleCountFlagBits m_MSAASamples, VkBool32 depthTest,
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants,
    VkRenderPass renderPass, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, VkPrimitiveTopology topology, float lineWidth)
{
    unsigned int* vertShaderCode = nullptr;
    unsigned int* fragShaderCode = nullptr;
    size_t vertShaderSize = ReadShaderFile(("res/shaders/spirv/" + vertexShader + ".spv").c_str(), &vertShaderCode);
    size_t fragShaderSize = ReadShaderFile(("res/shaders/spirv/" + fragmentShader + ".spv").c_str(), &fragShaderCode);

    VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode, vertShaderSize);
    VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode, fragShaderSize);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo
    {
        /* sType               */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        /* pNext               */ nullptr,
        /* flags               */ 0,
        /* stage               */ VK_SHADER_STAGE_VERTEX_BIT,
        /* module              */ vertShaderModule,
        /* pName               */ "main",
        /* pSpecializationInfo */ nullptr
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo
    {
        /* sType               */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        /* pNext               */ nullptr,
        /* flags               */ 0,
        /* stage               */ VK_SHADER_STAGE_FRAGMENT_BIT,
        /* module              */ fragShaderModule,
        /* pName               */ "main",
        /* pSpecializationInfo */ nullptr
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    VkPipelineVertexInputStateCreateInfo vertexInputInfo
    {
        /* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        /* pNext                           */ nullptr,
        /* flags                           */ 0,
        /* vertexBindingDescriptionCount   */ static_cast<uint32_t>(bindingDescriptions.size()),
        /* pVertexBindingDescriptions      */ bindingDescriptions.data(),
        /* vertexAttributeDescriptionCount */ static_cast<uint32_t>(attributeDescriptions.size()),
        /* pVertexAttributeDescriptions    */ attributeDescriptions.data()
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly
    {
        /* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        /* pNext                  */ nullptr,
        /* flags                  */ 0,
        /* topology               */ topology,
        /* primitiveRestartEnable */ VK_FALSE
    };


    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    if (lineWidth != 1.0f)
        dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

    VkPipelineDynamicStateCreateInfo dynamicState
    {
        /* sType             */ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        /* pNext             */ nullptr,
        /* flags             */ 0,
        /* dynamicStateCount */ (unsigned int)dynamicStates.size(),
        /* pDynamicStates    */ dynamicStates.data()
    };


    VkViewport viewport
    {
        /* x        */ 0.0f,
        /* y        */ 0.0f,
        /* width    */ static_cast<float>(viewportSize.x),
        /* height   */ static_cast<float>(viewportSize.y),
        /* minDepth */ 0.0f,
        /* maxDepth */ 1.0f
    };

    VkRect2D scissor
    {
        /* offset */ { 0, 0 },
        /* extent */ { viewportSize.x, viewportSize.y }
    };

    VkPipelineViewportStateCreateInfo viewportState
    {
        /* sType         */ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        /* pNext         */ nullptr,
        /* flags         */ 0,
        /* viewportCount */ 1,
        /* pViewports    */ &viewport,
        /* scissorCount  */ 1,
        /* pScissors     */ &scissor
    };


    VkPipelineRasterizationStateCreateInfo rasterizer
    {
        /* sType                   */ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        /* pNext                   */ nullptr,
        /* flags                   */ 0,
        /* depthClampEnable        */ VK_FALSE,
        /* rasterizerDiscardEnable */ VK_FALSE,
        /* polygonMode             */ VK_POLYGON_MODE_FILL,
        /* cullMode                */ VK_CULL_MODE_BACK_BIT,
        /* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
        /* depthBiasEnable         */ VK_FALSE,
        /* depthBiasConstantFactor */ 0.0f,
        /* depthBiasClamp          */ 0.0f,
        /* depthBiasSlopeFactor    */ 0.0f,
        /* lineWidth               */ lineWidth
    };


    VkPipelineMultisampleStateCreateInfo multisampling
    {
        /* sType                 */ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        /* pNext                 */ nullptr,
        /* flags                 */ 0,
        /* rasterizationSamples  */ m_MSAASamples,
        /* sampleShadingEnable   */ VK_TRUE,
        /* minSampleShading      */ 0.2f,
        /* pSampleMask           */ nullptr,
        /* alphaToCoverageEnable */ VK_FALSE,
        /* alphaToOneEnable      */ VK_FALSE
    };


    VkPipelineDepthStencilStateCreateInfo depthStencil
    {
        /* sType                 */ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        /* pNext                 */ nullptr,
        /* flags                 */ 0,
        /* depthTestEnable       */ depthTest,
        /* depthWriteEnable      */ depthTest,
        /* depthCompareOp        */ VK_COMPARE_OP_LESS,
        /* depthBoundsTestEnable */ VK_FALSE,
        /* stencilTestEnable     */ VK_FALSE,
        /* front                 */ {},
        /* back                  */ {},
        /* minDepthBounds        */ 0.0f,
        /* maxDepthBounds        */ 1.0f,
    };


    VkPipelineColorBlendAttachmentState colorBlendAttachment
    {
        /* blendEnable         */ VK_TRUE,
        /* srcColorBlendFactor */ VK_BLEND_FACTOR_SRC_ALPHA,
        /* dstColorBlendFactor */ VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        /* colorBlendOp        */ VK_BLEND_OP_ADD,
        /* srcAlphaBlendFactor */ VK_BLEND_FACTOR_ONE,
        /* dstAlphaBlendFactor */ VK_BLEND_FACTOR_ZERO,
        /* alphaBlendOp        */ VK_BLEND_OP_ADD,
        /* colorWriteMask      */ VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlending
    {
        /* sType             */ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        /* pNext             */ nullptr,
        /* flags             */ 0,
        /* logicOpEnable     */ VK_FALSE,
        /* logicOp           */ VK_LOGIC_OP_COPY,
        /* attachmentCount   */ 1,
        /* pAttachments      */ &colorBlendAttachment,
        /* blendConstants[4] */ { 0.0f, 0.0f, 0.0f, 0.0f }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo
    {
        /* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        /* pNext                  */ nullptr,
        /* flags                  */ 0,
        /* setLayoutCount         */ static_cast<uint32_t>(descriptorSetLayouts.size()),
        /* pSetLayouts            */ descriptorSetLayouts.data(),
        /* pushConstantRangeCount */ static_cast<uint32_t>(pushConstants.size()),
        /* pPushConstantRanges    */ pushConstants.data()
    };

    VK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));


    VkGraphicsPipelineCreateInfo pipelineInfo
    {
        /* sType               */ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        /* pNext               */ nullptr,
        /* flags               */ 0,
        /* stageCount          */ 2,
        /* pStages             */ shaderStages,
        /* pVertexInputState   */ &vertexInputInfo,
        /* pInputAssemblyState */ &inputAssembly,
        /* pTessellationState  */ nullptr,
        /* pViewportState      */ &viewportState,
        /* pRasterizationState */ &rasterizer,
        /* pMultisampleState   */ &multisampling,
        /* pDepthStencilState  */ &depthStencil,
        /* pColorBlendState    */ &colorBlending,
        /* pDynamicState       */ &dynamicState,
        /* layout              */ pipelineLayout,
        /* renderPass          */ renderPass,
        /* subpass             */ 0,
        /* basePipelineHandle  */ VK_NULL_HANDLE,
        /* basePipelineIndex   */ -1
    };

    VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));


    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    delete[] vertShaderCode;
    delete[] fragShaderCode;
}

void Shader::CreateComputePipeline(VkDevice device, const std::string& computeShader, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline)
{
    unsigned int* compShaderCode = nullptr;
    size_t compShaderSize = ReadShaderFile(("res/shaders/spirv/" + computeShader + ".spv").c_str(), &compShaderCode);

    VkShaderModule compShaderModule = CreateShaderModule(device, compShaderCode, compShaderSize);


    VkPipelineShaderStageCreateInfo compShaderStageInfo
    {
        /* sType               */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        /* pNext               */ nullptr,
        /* flags               */ 0,
        /* stage               */ VK_SHADER_STAGE_COMPUTE_BIT,
        /* module              */ compShaderModule,
        /* pName               */ "main",
        /* pSpecializationInfo */ nullptr
    };


    VkPipelineLayoutCreateInfo pipelineLayoutInfo
    {
        /* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        /* pNext                  */ nullptr,
        /* flags                  */ 0,
        /* setLayoutCount         */ static_cast<uint32_t>(descriptorSetLayouts.size()),
        /* pSetLayouts            */ descriptorSetLayouts.data(),
        /* pushConstantRangeCount */ static_cast<uint32_t>(pushConstants.size()),
        /* pPushConstantRanges    */ pushConstants.data()
    };

    VK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));


    VkComputePipelineCreateInfo pipelineInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        /* pNext              */ nullptr,
        /* flags              */ 0,
        /* stage              */ compShaderStageInfo,
        /* layout             */ pipelineLayout,
        /* basePipelineHandle */ VK_NULL_HANDLE,
        /* basePipelineIndex  */ -1
    };

    VK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));


    vkDestroyShaderModule(device, compShaderModule, nullptr);

    delete[] compShaderCode;
}

size_t Shader::ReadShaderFile(const std::string& filename, unsigned int** code)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        printf("Error open shader: %s\n", filename.c_str());
        return {};
    }

    size_t fileSize = (size_t)file.tellg();
    *code = (unsigned int*)malloc(fileSize * sizeof(unsigned int));

    file.seekg(0);
    file.read((char*)*code, fileSize);

    file.close();

    return fileSize;
}

VkShaderModule Shader::CreateShaderModule(VkDevice device, const unsigned int* data, size_t size)
{
    VkShaderModuleCreateInfo createInfo
    {
        /* sType    */ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        /* pNext    */ nullptr,
        /* flags    */ 0,
        /* codeSize */ size,
        /* pCode    */ data
    };

    VkShaderModule shaderModule;
    VK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}
