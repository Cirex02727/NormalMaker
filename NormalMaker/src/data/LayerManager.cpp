#include "vkpch.h"
#include "LayerManager.h"

#include "layer/VulkanLayer.h"

LayerManager::LayerManager(Application* application, MappedBuffer& uniformBuffer)
    : m_Application(application), m_UniformBuffer(uniformBuffer)
{
    std::vector<float> vertices =
    {
        //  Position //  //  UV  //
          0.0f, 0.0f,    0.0f, 0.0f,
          1.0f, 0.0f,    1.0f, 0.0f,
          1.0f, 1.0f,    1.0f, 1.0f,
                         
          1.0f, 1.0f,    1.0f, 1.0f,
          0.0f, 1.0f,    0.0f, 1.0f,
          0.0f, 0.0f,    0.0f, 0.0f
    };

    VkDevice device = m_Application->GetDevice();

    m_LayerVertexBuffer = Buffer::CreateDataBuffer(device, m_Application->GetPhysicalDevice(), m_Application->GetQueue(),
        m_Application->GetCommandPool(), vertices.data(), static_cast<uint32_t>(sizeof(LayerVertex) * vertices.size()),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


    VkRenderPass renderPass = m_Application->GetRenderPass();

    CreateDescriptorPool();
    CreateDescriptorSetLayout();


    VkPushConstantRange pushConstants
    {
        /* stageFlags */ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        /* offset     */ 0,
        /* size       */ sizeof(PushConstants)
    };
    Shader::CreateGraphicsPipeline(device, "layer.vert", "layer.frag", { LayerVertex::getBindingDescription() },
        { LayerVertex::getAttributeDescriptions() }, {}, m_Application->GetMSAASamples(), VK_TRUE,
        { m_DescriptorSetLayout }, { pushConstants }, renderPass, m_PipelineLayout, m_Pipeline);


    CreatePaintDescriptorSetLayout();

    VkPushConstantRange paintConstants
    {
        /* stageFlags */ VK_SHADER_STAGE_COMPUTE_BIT,
        /* offset     */ 0,
        /* size       */ sizeof(PaintConstants)
    };
    Shader::CreateComputePipeline(device, "paint.comp", { m_PaintDescriptorSetLayout },
        { paintConstants }, m_PaintPipelineLayout, m_PaintPipeline);


    CreateCombineDescriptorSetLayout();

    VkPushConstantRange combineConstants
    {
        /* stageFlags */ VK_SHADER_STAGE_COMPUTE_BIT,
        /* offset     */ 0,
        /* size       */ sizeof(CombineConstants)
    };
    Shader::CreateComputePipeline(device, "combine.comp", { m_CombineDescriptorSetLayout },
        { combineConstants }, m_CombinePipelineLayout, m_CombinePipeline);
}

void LayerManager::Delete()
{
    VkDevice device = m_Application->GetDevice();


    ClearLayers(device);

    vkDestroyPipeline(device, m_CombinePipeline, nullptr);
    vkDestroyPipelineLayout(device, m_CombinePipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(device, m_CombineDescriptorSetLayout, nullptr);

    vkDestroyPipeline(device, m_PaintPipeline, nullptr);
    vkDestroyPipelineLayout(device, m_PaintPipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(device, m_PaintDescriptorSetLayout, nullptr);

    vkDestroyPipeline(device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);

    DeleteBuffer(device, m_LayerVertexBuffer);
}

void LayerManager::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, const glm::ivec2& canvasSize) const
{
    if (m_Layers.size() <= 0)
        return;

    // Render all layers
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_LayerVertexBuffer.Buffer, offsets);

    for (const Layer& layer : m_Layers)
    {
        PushConstants pushConstants
        {
            /* Position   */ layer.Position,
            /* Size       */ layer.Texture->GetSize(),
            /* CanvasSize */ canvasSize,

            /* ZOff       */ layer.ZOff,
            /* Padding    */ 0,
            /* Aplha      */ layer.Alpha,
        };

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &layer.DescriptorSet, 0, nullptr);

        vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PushConstants), &pushConstants);

        vkCmdDraw(commandBuffer, 3 * 2, 1, 0, 0);
    }
}

bool LayerManager::AddLayerFromFile(const std::string& filepath, glm::ivec2& canvasSize)
{
    VkDevice device = m_Application->GetDevice();
    VkPhysicalDevice physicalDevice = m_Application->GetPhysicalDevice();

	Layer& layer = m_Layers.emplace_back(
        std::make_unique<Texture>(device, physicalDevice, m_Application->GetQueue(), m_Application->GetCommandPool(),
            filepath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, true),
        nullptr, glm::ivec2{}, GetMaxZOff(), "Layer (" + std::to_string(m_Layers.size()) + ")");

    layer.Texture->CreateSampler(device, physicalDevice, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    bool updateCanvasSize = m_Layers.size() == 1;
    if(updateCanvasSize)
        canvasSize = layer.Texture->GetSize();

    CreateDescriptorSet(layer);

    return updateCanvasSize;
}

void LayerManager::AddNormalLayer(int width, int height)
{
    VkDevice device = m_Application->GetDevice();
    VkPhysicalDevice physicalDevice = m_Application->GetPhysicalDevice();

    Layer& layer = m_Layers.emplace_back(
        std::make_unique<Texture>(device, physicalDevice, m_Application->GetQueue(), m_Application->GetCommandPool(),
            width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, true),
        nullptr, glm::ivec2{}, GetMaxZOff(), "Layer (" + std::to_string(m_Layers.size()) + ")", 1.0f, true);

    layer.Texture->CreateSampler(device, physicalDevice, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    CreateDescriptorSet(layer);
}

void LayerManager::ClearLayers(VkDevice device)
{
    VkQueue queue = m_Application->GetQueue();
    VkCommandPool commandPool = m_Application->GetCommandPool();

    for (const Layer& layer : m_Layers)
    {
        Image::Barrier(device, queue, commandPool, layer.Texture->GetImage(), layer.Texture->GetFormat(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer.Texture->GetMipLevels());

        layer.Texture->Delete(device);
    }

    m_Layers.clear();

    ClearCurrPaintLayer(device);
}

void LayerManager::PaintLayer(uint32_t layerId, const glm::ivec2& imageSize, const glm::ivec2& position, int radius, const glm::vec4& color)
{
    if (layerId == -1 || layerId >= m_Layers.size())
        return;

    VkDevice device = m_Application->GetDevice();

    Layer& layer = m_Layers[layerId];

    if (m_CurrentPaintLayer != layerId)
    {
        ClearCurrPaintLayer(device);

        CreatePaintDescriptorSet(layer);
        m_CurrentPaintLayer = layerId;
    }

    // Dispatch Paint
    {
        VkCommandPool commandPool = m_Application->GetCommandPool();

        VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

        Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, layer.Texture->GetMipLevels());

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PaintPipelineLayout, 0, 1, &m_PaintDescriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PaintPipeline);

        PaintConstants paintConstants
        {
            /* Color         */ color,
            /* ImageSize     */ imageSize,
            /* Position      */ layer.Position - position,
            /* Radius        */ radius
        };
        vkCmdPushConstants(commandBuffer, m_PaintPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PaintConstants), &paintConstants);

        const glm::uvec2 dispatchSize = glm::ceil(glm::vec2(imageSize) / 16.0f);
        vkCmdDispatch(commandBuffer, dispatchSize.x, dispatchSize.y, 1);

        Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer.Texture->GetMipLevels());

        VK::EndSingleTimeCommands(device, m_Application->GetQueue(), commandPool, commandBuffer);
    }
}

void LayerManager::ClearCurrPaintLayer(VkDevice device)
{
    if (m_CurrentPaintLayer != -1)
        vkFreeDescriptorSets(device, m_DescriptorPool, 1, &m_PaintDescriptorSet);

    m_CurrentPaintLayer = -1;
}

void LayerManager::CombineLayers(const std::string& filepath, const glm::ivec2& canvasSize)
{
    VkDevice device = m_Application->GetDevice();
    VkPhysicalDevice physicalDevice = m_Application->GetPhysicalDevice();
    VkQueue queue = m_Application->GetQueue();
    VkCommandPool commandPool = m_Application->GetCommandPool();

    // Create Out Image
    Texture outTexture(device, physicalDevice, queue, commandPool, canvasSize.x, canvasSize.y, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, true);

    {
        VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

        Image::TransitionImageLayout(commandBuffer, outTexture.GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, outTexture.GetMipLevels());

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CombinePipeline);

        int i = 0;
        for (const Layer& layer : m_Layers)
        {
            Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, layer.Texture->GetMipLevels());

            if (i > 0)
                vkFreeDescriptorSets(device, m_DescriptorPool, 1, &m_CombineDescriptorSet);

            CreateCombineDescriptorSet(outTexture, layer);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CombinePipelineLayout, 0, 1, &m_CombineDescriptorSet, 0, nullptr);

            CombineConstants combineConstants
            {
                /* ImageSize */ canvasSize,
                /* Position  */ layer.Position
            };
            vkCmdPushConstants(commandBuffer, m_CombinePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CombineConstants), &combineConstants);

            const glm::uvec2 dispatchSize = glm::ceil(glm::vec2(canvasSize) / 16.0f);
            vkCmdDispatch(commandBuffer, dispatchSize.x, dispatchSize.y, 1);

            Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer.Texture->GetMipLevels());

            VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);

            if (i < m_Layers.size() - 1)
            {
                commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CombinePipeline);
            }

            ++i;
        }
    }

    // Read Image
    unsigned char* image = Image::Read(device, physicalDevice, queue, commandPool, outTexture.GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
        outTexture.GetWidth(), outTexture.GetHeight(), outTexture.GetMipLevels(), VK_IMAGE_LAYOUT_GENERAL);

    int succ = stbi_write_png(filepath.c_str(), outTexture.GetWidth(), outTexture.GetHeight(), 4, image,
        sizeof(unsigned char) * 4 * outTexture.GetWidth());

    delete[] image;

    // Wait and destroy Out Image
    Image::Barrier(device, queue, commandPool, outTexture.GetImage(), outTexture.GetFormat(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, outTexture.GetMipLevels());

    vkFreeDescriptorSets(device, m_DescriptorPool, 1, &m_CombineDescriptorSet);

    outTexture.Delete(device);
}

void LayerManager::SaveLayers(std::ofstream& out)
{
    VkDevice device = m_Application->GetDevice();
    VkPhysicalDevice physicalDevice = m_Application->GetPhysicalDevice();
    VkQueue queue = m_Application->GetQueue();
    VkCommandPool commandPool = m_Application->GetCommandPool();


    uint32_t layerCount = static_cast<uint32_t>(m_Layers.size());
    out.write((char*)&layerCount, sizeof(uint32_t));

    for (const Layer& layer : m_Layers)
    {
        // Write metadata
        out.write((char*)&layer.Position.x, sizeof(glm::ivec2));
        out.write((char*)&layer.ZOff, sizeof(float));

        uint32_t nameSize = static_cast<uint32_t>(sizeof(char) * layer.Name.size());
        out.write((char*)&nameSize, sizeof(uint32_t));
        out.write(layer.Name.data(), nameSize);

        out.write((char*)&layer.Alpha, sizeof(float));
        out.write((char*)&layer.IsNormal, sizeof(bool));

        // Write Image
        uint32_t width = layer.Texture->GetWidth();
        uint32_t height = layer.Texture->GetHeight();
        unsigned char* image = Image::Read(device, physicalDevice, queue, commandPool, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            width, height, layer.Texture->GetMipLevels(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        stbi_write_png_to_func([](void* context, void* data, int size)
            {
                std::ofstream* out = (std::ofstream*)context;

                out->write((char*)&size, sizeof(int));
                out->write((char*)data, size);
            }, &out, width, height, 4, image, sizeof(unsigned char) * 4 * width);


        // Reset Image Layout
        Image::TransitionImageLayout(device, queue, commandPool, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer.Texture->GetMipLevels());

        delete[] image;
    }
}

void LayerManager::LoadLayers(std::ifstream& in)
{
    VkDevice device = m_Application->GetDevice();
    VkPhysicalDevice physicalDevice = m_Application->GetPhysicalDevice();
    VkQueue queue = m_Application->GetQueue();
    VkCommandPool commandPool = m_Application->GetCommandPool();


    uint32_t layerCount = 0;
    in.read((char*)&layerCount, sizeof(uint32_t));

    for (uint32_t i = 0; i < layerCount; ++i)
    {
        // Read metadata
        glm::ivec2 position{};
        in.read((char*)&position.x, sizeof(glm::ivec2));

        float zOff = 0.0f;
        in.read((char*)&zOff, sizeof(float));

        uint32_t nameSize = 0;
        in.read((char*)&nameSize, sizeof(uint32_t));

        std::string name(nameSize, '\0');
        in.read(name.data(), nameSize);

        float alpha = 0.0f;
        in.read((char*)&alpha, sizeof(float));

        bool isNormal = false;
        in.read((char*)&isNormal, sizeof(bool));

        // Read Image
        int size = 0;
        in.read((char*)&size, sizeof(int));

        unsigned char* buff = new unsigned char[size];
        in.read((char*)buff, size);

        int width = -1;
        int height = -1;
        int components = -1;
        stbi_uc* image = stbi_load_from_memory((const stbi_uc*)buff, size, &width, &height, &components, 4);

        delete[] buff;

        Layer& layer = m_Layers.emplace_back(
            std::make_unique<Texture>(device, physicalDevice, queue, commandPool, image, width, height, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, true),
            nullptr, position, zOff, name, alpha, isNormal);

        layer.Texture->CreateSampler(device, physicalDevice, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

        CreateDescriptorSet(layer);

        delete[] image;
    }
}

void LayerManager::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount */ MAX_LAYERS
        },
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            /* descriptorCount */ MAX_LAYERS
        },
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount */ 2
        }
    };

    VkDescriptorPoolCreateInfo poolInfo
    {
        /* sType         */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext         */ nullptr,
        /* flags         */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        /* maxSets       */ MAX_LAYERS * 2 + 2,
        /* poolSizeCount */ static_cast<uint32_t>(poolSizes.size()),
        /* pPoolSizes    */ poolSizes.data()
    };

    VK(vkCreateDescriptorPool(m_Application->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool));
}

void LayerManager::CreateDescriptorSetLayout()
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
        },
        VkDescriptorSetLayoutBinding
        {
            /* binding            */ 1,
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            /* descriptorCount    */ 1,
            /* stageFlags         */ VK_SHADER_STAGE_FRAGMENT_BIT,
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

    VK(vkCreateDescriptorSetLayout(m_Application->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout));
}

void LayerManager::CreateDescriptorSet(Layer& layer)
{
    VkDevice device = m_Application->GetDevice();

    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_DescriptorPool,
        /* descriptorSetCount */ 1,
        /* pSetLayouts        */ &m_DescriptorSetLayout
    };
    VK(vkAllocateDescriptorSets(device, &allocInfo, &layer.DescriptorSet));


    VkDescriptorBufferInfo bufferInfo
    {
        /* buffer */ m_UniformBuffer.Buffer,
        /* offset */ 0,
        /* range  */ sizeof(UniformBufferObject)
    };

    VkDescriptorImageInfo layerInfo
    {
        /* sampler     */ layer.Texture->GetSampler(),
        /* imageView   */ layer.Texture->GetView(),
        /* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites =
    {
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ layer.DescriptorSet,
            /* dstBinding       */ 0,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* pImageInfo       */ nullptr,
            /* pBufferInfo      */ &bufferInfo,
            /* pTexelBufferView */ nullptr
        },
        VkWriteDescriptorSet
            {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ layer.DescriptorSet,
            /* dstBinding       */ 1,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            /* pImageInfo       */ &layerInfo,
            /* pBufferInfo      */ nullptr,
            /* pTexelBufferView */ nullptr
        }
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void LayerManager::CreatePaintDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
    {
        VkDescriptorSetLayoutBinding
        {
            /* binding            */ 0,
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount    */ 1,
            /* stageFlags         */ VK_SHADER_STAGE_COMPUTE_BIT,
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

    VK(vkCreateDescriptorSetLayout(m_Application->GetDevice(), &layoutInfo, nullptr, &m_PaintDescriptorSetLayout));
}

void LayerManager::CreatePaintDescriptorSet(Layer& layer)
{
    VkDevice device = m_Application->GetDevice();

    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_DescriptorPool,
        /* descriptorSetCount */ 1,
        /* pSetLayouts        */ &m_PaintDescriptorSetLayout
    };
    VK(vkAllocateDescriptorSets(device, &allocInfo, &m_PaintDescriptorSet));


    VkDescriptorImageInfo layerInfo
    {
        /* sampler     */ nullptr,
        /* imageView   */ layer.Texture->GetView(),
        /* imageLayout */ VK_IMAGE_LAYOUT_GENERAL
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites =
    {
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ m_PaintDescriptorSet,
            /* dstBinding       */ 0,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo       */ &layerInfo,
            /* pBufferInfo      */ nullptr,
            /* pTexelBufferView */ nullptr
        }
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void LayerManager::CreateCombineDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
    {
        VkDescriptorSetLayoutBinding
        {
            /* binding            */ 0,
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount    */ 1,
            /* stageFlags         */ VK_SHADER_STAGE_COMPUTE_BIT,
            /* pImmutableSamplers */ nullptr
        },
        VkDescriptorSetLayoutBinding
        {
            /* binding            */ 1,
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount    */ 1,
            /* stageFlags         */ VK_SHADER_STAGE_COMPUTE_BIT,
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

    VK(vkCreateDescriptorSetLayout(m_Application->GetDevice(), &layoutInfo, nullptr, &m_CombineDescriptorSetLayout));
}

void LayerManager::CreateCombineDescriptorSet(const Texture& outTexture, const Layer& layer)
{
    VkDevice device = m_Application->GetDevice();

    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_DescriptorPool,
        /* descriptorSetCount */ 1,
        /* pSetLayouts        */ &m_CombineDescriptorSetLayout
    };
    VK(vkAllocateDescriptorSets(device, &allocInfo, &m_CombineDescriptorSet));


    VkDescriptorImageInfo outTextureInfo
    {
        /* sampler     */ nullptr,
        /* imageView   */ outTexture.GetView(),
        /* imageLayout */ VK_IMAGE_LAYOUT_GENERAL
    };

    VkDescriptorImageInfo layerInfo
    {
        /* sampler     */ nullptr,
        /* imageView   */ layer.Texture->GetView(),
        /* imageLayout */ VK_IMAGE_LAYOUT_GENERAL
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites =
    {
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ m_CombineDescriptorSet,
            /* dstBinding       */ 0,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo       */ & outTextureInfo,
            /* pBufferInfo      */ nullptr,
            /* pTexelBufferView */ nullptr
        },
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ m_CombineDescriptorSet,
            /* dstBinding       */ 1,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo       */ &layerInfo,
            /* pBufferInfo      */ nullptr,
            /* pTexelBufferView */ nullptr
        }
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}
