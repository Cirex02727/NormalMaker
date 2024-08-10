#include "vkpch.h"
#include "VulkanLayer.h"

#include "core/Core.h"

#include "utils/ColorManager.h"

void VulkanLayer::Init(Application& application)
{
    m_Application = &application;

    m_Device                  = application.GetDevice();
    m_PhysicalDevice          = application.GetPhysicalDevice();
    m_Queue                   = application.GetQueue();
    m_CommandPool             = application.GetCommandPool();
    m_ViewportSize            = application.GetViewportSize();
    m_PrevViewportRegionAvail = application.GetViewportRegionAvail();
    m_RenderPass              = application.GetRenderPass();
    m_SwapChainImageFormat    = application.GetSwapChainImageFormat();
    m_MSAASamples             = application.GetMSAASamples();
    m_DepthImageView          = application.GetDepthImageView();
    m_ViewportImageSampler    = application.GetViewportImageSampler();
    m_ImageCount              = application.GetImageCount();
    m_ImGuiDescriptorPool     = application.GetImGuiDescriptorPool();

    m_Camera = std::make_shared<Camera>(m_Application);
    m_Camera->LoadSettings();
    m_Camera->OnResize(m_PrevViewportRegionAvail.x, m_PrevViewportRegionAvail.y);

    CreateUniformBuffer();

    m_LayerManager = std::make_unique<LayerManager>(m_Application, m_UniformBuffer);


    // Debug Lines
    m_GridRenderer = std::make_unique<DebugRenderer>(m_Device, m_PhysicalDevice, m_UniformBuffer, m_MSAASamples, m_RenderPass, 16384);
    m_NormalArrowsRenderer = std::make_unique<DebugRenderer>(m_Device, m_PhysicalDevice, m_UniformBuffer, m_MSAASamples, m_RenderPass, 1024, 3.0f);


    CreateNormalArrowsUniformBuffer();

    CreateDescriptorPool();

    // Normal
    CreateNormalDescriptorSetLayout();

    VkPushConstantRange paintConstants
    {
        /* stageFlags */ VK_SHADER_STAGE_COMPUTE_BIT,
        /* offset     */ 0,
        /* size       */ sizeof(glm::ivec2)
    };

    Shader::CreateComputePipeline(m_Device, "normal.comp", { m_NormalDescriptorSetLayout },
        { paintConstants }, m_NormalPipelineLayout, m_NormalPipeline);


    // Load Settings
    YAML::Node global = SaveManager::GetNode("Global");
    if(global["GridDepth"].IsDefined())   m_GridDepth   = global["GridDepth"].as<float>();
    if(global["BrushRadius"].IsDefined()) m_BrushRadius = global["BrushRadius"].as<int>();


    // Tests //
    if(false)
    {
        m_CurrProject = "C:\\Users\\SamyPC\\source\\repos\\NormalMaker\\NormalMaker\\res\\projects\\project0.nm";
        m_IsProjectLoaded = true;

        LoadProject();

        m_SelectedLayer = 1;

        const Layer& layer = m_LayerManager->GetLayers()[m_SelectedLayer];
        CreateNormalDescriptorSet(layer);

        // ----- //
        DispatchNormal(layer);
    }
}

void VulkanLayer::CreateUniformBuffer()
{
    const uint32_t bufferSize = sizeof(UniformBufferObject);

    m_UniformBuffer = Buffer::CreateMappedBuffer(m_Device, m_PhysicalDevice,
        bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void VulkanLayer::UpdateUniformBuffer() const
{
    UniformBufferObject ubo
    {
        /* view  */ m_Camera->GetView(),
        /* ortho */ m_Camera->GetOrtho(),
    };

    memcpy(m_UniformBuffer.Map, &ubo, sizeof(ubo));
}

void VulkanLayer::CreateNormalArrowsUniformBuffer()
{
    m_NormalArrowsUniformBuffer = Buffer::CreateMappedBuffer(m_Device, m_PhysicalDevice, static_cast<uint32_t>(sizeof(NormalArrows)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void VulkanLayer::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount */ 1
        },
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
        /* flags         */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        /* maxSets       */ 1,
        /* poolSizeCount */ static_cast<uint32_t>(poolSizes.size()),
        /* pPoolSizes    */ poolSizes.data()
    };

    VK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));
}

void VulkanLayer::CreateNormalDescriptorSetLayout()
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
            /* descriptorType     */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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

    VK(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_NormalDescriptorSetLayout));
}

void VulkanLayer::CreateNormalDescriptorSet(const Layer& layer)
{
    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_DescriptorPool,
        /* descriptorSetCount */ 1,
        /* pSetLayouts        */ &m_NormalDescriptorSetLayout
    };
    VK(vkAllocateDescriptorSets(m_Device, &allocInfo, &m_NormalDescriptorSet));


    VkDescriptorImageInfo layerInfo
    {
        /* sampler     */ nullptr,
        /* imageView   */ layer.Texture->GetView(),
        /* imageLayout */ VK_IMAGE_LAYOUT_GENERAL
    };

    VkDescriptorBufferInfo bufferInfo
    {
        /* buffer */ m_NormalArrowsUniformBuffer.Buffer,
        /* offset */ 0,
        /* range  */ sizeof(NormalArrows)
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites =
    {
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ m_NormalDescriptorSet,
            /* dstBinding       */ 0,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo       */ &layerInfo,
            /* pBufferInfo      */ nullptr,
            /* pTexelBufferView */ nullptr
        },
        VkWriteDescriptorSet
        {
            /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext            */ nullptr,
            /* dstSet           */ m_NormalDescriptorSet,
            /* dstBinding       */ 1,
            /* dstArrayElement  */ 0,
            /* descriptorCount  */ 1,
            /* descriptorType   */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* pImageInfo       */ nullptr,
            /* pBufferInfo      */ &bufferInfo,
            /* pTexelBufferView */ nullptr
        }
    };

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanLayer::Destroy()
{
    vkDestroyPipeline(m_Device, m_NormalPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_NormalPipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(m_Device, m_NormalDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

    DeleteBuffer(m_Device, m_NormalArrowsUniformBuffer);

    m_NormalArrowsRenderer->Delete(m_Device);
    m_GridRenderer->Delete(m_Device);

    m_LayerManager->Delete();

    DeleteBuffer(m_Device, m_UniformBuffer);

    YAML::Node global = SaveManager::GetNode("Global");
    global["GridDepth"]   = m_GridDepth;
    global["BrushRadius"] = m_BrushRadius;

    m_Camera->SaveSettings();
}

void VulkanLayer::OnUpdate(float dt)
{
    glm::uvec2 viewportRegionAvail = m_Application->GetViewportRegionAvail();
    if (viewportRegionAvail != m_PrevViewportRegionAvail)
    {
        m_Camera->OnResize(viewportRegionAvail.x, viewportRegionAvail.y);
        m_PrevViewportRegionAvail = viewportRegionAvail;
    }

	m_Camera->OnUpdate(dt);


    const glm::vec2 point = GetMouseWorldPosition();

    // Paint on current layer
    if (Input::IsMouse(MouseButton::Left) && m_Application->IsViewportHovered())
    {
        const glm::ivec2 imagePoint = glm::ivec2(point);

        constexpr glm::vec4 clearColor       = { 0.0f, 0.0f, 0.0f, 0.0f };
        constexpr glm::vec4 normalBrushColor = { 0.5f, 0.5f, 0.5f, 1.0f };

        if(imagePoint.x >= 0 && imagePoint.y >= 0 && imagePoint.x < m_CanvasSize.x && imagePoint.y < m_CanvasSize.y)
            m_LayerManager->PaintLayer(m_SelectedLayer, m_CanvasSize, imagePoint, m_BrushRadius,
                m_UseEraser ? clearColor : (m_UseNormalBrush ? normalBrushColor : m_BrushColor));
    }

    if (Input::IsMouseDown(MouseButton::Right))
    {
        m_NormalArrows.Arrows[m_NormalArrows.Count++] = NormalArrow{ point, point + glm::vec2(0.0f, 20.0f), glm::radians(0.001f) };
        m_IsMovingNormalArrow = true;

        DrawNormalArrows();
    }

    if (Input::IsMouseUp(MouseButton::Right))
        m_IsMovingNormalArrow = false;

    if (m_IsMovingNormalArrow)
    {
        constexpr float maxDist = 20.0f;

        NormalArrow& arrow = m_NormalArrows.Arrows[m_NormalArrows.Count - 1];
        if(point != arrow.Start)
            arrow.End = arrow.Start + glm::normalize(point - arrow.Start) * maxDist;

        DrawNormalArrows();
    }
}

void VulkanLayer::OnImGuiRenderMenuBar(bool& isRunning)
{
    if (ImGui::BeginMenu("App"))
    {
        if (ImGui::MenuItem("Close"))
            isRunning = false;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Project"))
    {
        nfdchar_t* outPath;
        if (ImGui::MenuItem("New Project", "CTRL+N") && NFD_SaveDialog("nm", "", &outPath) == NFD_OKAY)
        {
            m_CurrProject = outPath;
            m_IsProjectLoaded = true;

            ClearProject();
        }

        if (ImGui::MenuItem("Import Project", "CTRL+I") && NFD_OpenDialog("nm", "", &outPath) == NFD_OKAY)
        {
            ClearProject(m_IsProjectLoaded);

            m_CurrProject = outPath;
            m_IsProjectLoaded = true;

            LoadProject();

            m_Camera->SetPosition(glm::vec3(-m_CanvasSize / 2, 0.0f));
            m_Camera->SetZoom((m_CanvasSize.x > m_CanvasSize.y ? m_CanvasSize.x : m_CanvasSize.y) / 2.0f
                * ((float)m_PrevViewportRegionAvail.y / m_PrevViewportRegionAvail.x) + 20);
        }

        if (m_IsProjectLoaded && ImGui::MenuItem("Save Project", "CTRL+S"))
        {
            SaveProject();
        }

        ImGui::EndMenu();
    }

    if (m_IsProjectLoaded && ImGui::BeginMenu("Image"))
    {
        if (ImGui::MenuItem("Import Image"))
        {
            nfdchar_t* outPath;
            nfdresult_t res = NFD_OpenDialog("png,jpg,jpeg", "", &outPath);
            if (res == NFD_OKAY && m_LayerManager->AddLayerFromFile(outPath, m_CanvasSize))
            {
                ClearProject(false);
                BuildGrid();

                m_Camera->SetPosition(glm::vec3(-m_CanvasSize / 2, 0.0f));
                m_Camera->SetZoom((m_CanvasSize.x > m_CanvasSize.y ? m_CanvasSize.x : m_CanvasSize.y) / 2.0f
                    * ((float)m_PrevViewportRegionAvail.y / m_PrevViewportRegionAvail.x) + 20);
            }
        }

        if (m_IsProjectLoaded && ImGui::MenuItem("Export PNG"))
        {
            nfdchar_t* outPath;
            nfdresult_t res = NFD_SaveDialog("png", "", &outPath);
            if (res == NFD_OKAY)
                m_LayerManager->CombineLayers(outPath, m_CanvasSize);
        }

        ImGui::EndMenu();
    }
}

void VulkanLayer::OnImGuiRender()
{
    ImGui::Begin("Scene");
    {
        if (ImGui::DragFloat2("Camera Pos", &m_Camera->GetPositionR().x))
            m_Camera->RecalculateView();

        ImGui::DragFloat("Grid Depth", &m_GridDepth, 0.1f);

        ImGui::Checkbox("Use Normal Brush", &m_UseNormalBrush);

        ImGui::Checkbox("Use Eraser", &m_UseEraser);

        const glm::vec2 point = GetMouseWorldPosition();
        ImGui::Text(("Cursor Position (" + std::format("{:.1f}", point.x) + "; " + std::format("{:.1f}", point.y) + ")").c_str());
    }
    ImGui::End();

    ImGui::Begin("Brush");
    {
        ImGui::DragInt("Radius", &m_BrushRadius, 1, 1, 100);
        ImGui::ColorEdit3("Color", &m_BrushColor.x);
    }
    ImGui::End();

    ImGui::Begin("Layers");
    {
        std::vector<Layer>& layers = m_LayerManager->GetLayers();

        ImVec2 freeSpace = ImGui::GetContentRegionAvail();

        int i = 0, toRemove = -1;
        for (Layer& layer : layers)
        {
            ImGui::PushID(i);
            
            bool isSelected = i == m_SelectedLayer;
            ImGui::Text((layer.Name + (isSelected ? " - Selected" : "")).c_str());

            ImGui::DragInt2 ("Position", &layer.Position.x);
            ImGui::DragFloat("ZOff",     &layer.ZOff);

            ImGui::DragFloat("Alpha",    &layer.Alpha, 0.01f, 0.0f, 1.0f);

            if (layer.IsNormal)
            {
                if (ImGui::Button("Delete", ImVec2{ freeSpace.x / 2, 0 }))
                    toRemove = i;

                ImGui::SameLine();

                if (!isSelected && ImGui::Button("Select", ImVec2{ freeSpace.x / 2, 0 }))
                {
                    if(m_SelectedLayer != -1)
                        vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_NormalDescriptorSet);

                    m_SelectedLayer = i;

                    CreateNormalDescriptorSet(layer);
                }
                
                if (isSelected && ImGui::Button("Deselect", ImVec2{ freeSpace.x / 2, 0 }))
                {
                    if (m_SelectedLayer != -1)
                        vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_NormalDescriptorSet);

                    m_SelectedLayer = -1;
                }
            }
            else
                if (ImGui::Button("Delete", ImVec2{ freeSpace.x, 0 }))
                    toRemove = i;

            ImGui::Separator();

            ImGui::PopID();
            ++i;
        }

        if (toRemove != -1)
        {
            if(m_SelectedLayer != -1)
                vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_NormalDescriptorSet);

            m_SelectedLayer = -1;

            m_LayerManager->ClearCurrPaintLayer(m_Device);

            std::unique_ptr<Texture>& texture = layers[toRemove].Texture;
            Image::Barrier(m_Device, m_Queue, m_CommandPool, texture->GetImage(), texture->GetFormat(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture->GetMipLevels());

            texture->Delete(m_Device);

            layers.erase(layers.begin() + toRemove);

            if (layers.size() == 0)
                m_GridRenderer->ClearLines();
        }

        ImGui::Separator();

        if (m_IsProjectLoaded && m_CanvasSize.x > 0 && m_CanvasSize.y > 0 &&
            ImGui::Button("New", ImVec2{ freeSpace.x, 0 }))
        {
            if (layers.size() == 0)
                m_SelectedLayer = -1;

            m_LayerManager->AddNormalLayer(m_CanvasSize.x, m_CanvasSize.y);
        }
    }
    ImGui::End();

    ImGui::Begin("Normal Arrows");
    {
        ImVec2 freeSpace = ImGui::GetContentRegionAvail();

        if (m_IsProjectLoaded && m_NormalArrows.Count > 0)
        {
            ImGui::Text(("Selected Layer: " + std::to_string(m_SelectedLayer)).c_str());

            if (m_SelectedLayer != -1 && ImGui::Button("Calculate Normals"))
                DispatchNormal(m_LayerManager->GetLayers()[m_SelectedLayer]);

            ImGui::Separator();

            if (ImGui::Button("Clear Normal Arrows"))
            {
                m_NormalArrows.Count = 0;
                m_NormalArrowsRenderer->ClearLines();
            }

            ImGui::Separator();
        }

        int toRemove = -1;
        for (int i = 0; i < m_NormalArrows.Count; ++i)
        {
            NormalArrow& arrow = m_NormalArrows.Arrows[i];

            ImGui::PushID(i);

            bool isSelected = i == m_SelectedNormalArrow;
            ImGui::Text(("Arrow (" + std::to_string(i) + ")" + (isSelected ? " - Selected" : "")).c_str());

            ImGui::Text(("Start (" + std::to_string(arrow.Start.x) + "; " + std::to_string(arrow.Start.y) + ")").c_str());
            ImGui::Text(("End   (" + std::to_string(arrow.End.x) + "; " + std::to_string(arrow.End.y) + ")").c_str());

            {
                const glm::vec2 v = { arrow.End.x - arrow.Start.x, arrow.Start.y - arrow.End.y };
                float orientation = glm::degrees(std::atan(v.y / v.x)) + (v.x < 0.0f ? 180.0f : (v.y < 0.0f ? 360.0f : 0.0f));

                if (ImGui::DragFloat("Orientation", &orientation, 0.1f, -360.0f, 360.0f))
                {
                    if (orientation < 0)
                        orientation += 360;
                    else
                        orientation -= ((int)orientation % 360) * 360;

                    orientation = glm::radians(orientation);
                    
                    const glm::vec2 t{ std::cos(orientation), -std::sin(orientation) };
                    arrow.End = arrow.Start + t * 20.0f;

                    DrawNormalArrows();
                }
            }

            float angle = glm::degrees(arrow.Angle);
            if (ImGui::DragFloat("Angle", &angle, 0.1f, 0.001f, 89.999f))
                arrow.Angle = glm::radians(std::clamp(angle, 0.001f, 89.999f));

            if (ImGui::Button("Delete", ImVec2{ freeSpace.x / 2, 0 }))
                toRemove = i;

            ImGui::SameLine();

            if (!isSelected && ImGui::Button("Select", ImVec2{ freeSpace.x / 2, 0 }))
            {
                m_SelectedNormalArrow = i;

                DrawNormalArrows();
            }

            if (isSelected && ImGui::Button("Deselect", ImVec2{ freeSpace.x / 2, 0 }))
            {
                m_SelectedNormalArrow = -1;

                DrawNormalArrows();
            }

            ImGui::Separator();

            ImGui::PopID();
        }

        if (toRemove != -1)
        {
            if (toRemove < m_NormalArrows.Count - 1)
                memcpy(m_NormalArrows.Arrows + toRemove, m_NormalArrows.Arrows + toRemove + 1, sizeof(NormalArrow) * (size_t)(m_NormalArrows.Count - 1));

            --m_NormalArrows.Count;

            if (m_SelectedNormalArrow >= m_NormalArrows.Count && m_SelectedNormalArrow >= 0)
                m_SelectedNormalArrow = m_NormalArrows.Count > 0 ? static_cast<int>(m_NormalArrows.Count - 1) : -1;

            DrawNormalArrows();
        }
    }
    ImGui::End();
}

void VulkanLayer::OnPreRender(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    // Render Tilemaps
    UpdateUniformBuffer();
}

void VulkanLayer::OnRender(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    // Render all layers
    m_LayerManager->Render(commandBuffer, imageIndex, m_CanvasSize);

    // Render debug grid lines
    {
        float smoothRange = 175.0f;
        float t = std::clamp((m_Camera->GetZoom() - m_GridDepth) / ((m_GridDepth + smoothRange) - m_GridDepth), 0.0f, 1.0f);
        float alpha = 1.0f - (t * t * (3.0f - 2.0f * t));
        m_GridRenderer->Render(commandBuffer, alpha);
    }

    m_NormalArrowsRenderer->Render(commandBuffer, 1.0f);
}

void VulkanLayer::OnPostRender(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{

}

void VulkanLayer::OnResize(const glm::uvec2& size)
{
    m_ViewportSize = size;

    m_DepthImageView = m_Application->GetDepthImageView();

    m_ViewportImageSampler = m_Application->GetViewportImageSampler();
}

void VulkanLayer::DrawNormalArrows()
{
    constexpr glm::vec3 color(1.0f);
    constexpr glm::vec3 selected(0.8f, 0.3f, 0.2f);

    std::vector<DebugRendererVertex> lines;

    for (int i = 0; i < m_NormalArrows.Count; ++i)
        CalculateNormalArrow(lines, m_NormalArrows.Arrows[i], i == m_SelectedNormalArrow ? selected : color);

    m_NormalArrowsRenderer->ClearLines();
    m_NormalArrowsRenderer->AddLines(lines);
}

void VulkanLayer::CalculateNormalArrow(std::vector<DebugRendererVertex>& lines, const NormalArrow& arrow, const glm::vec3& color) const
{
    lines.emplace_back(arrow.Start, color);
    lines.emplace_back(arrow.End, color);

    glm::vec2 start = arrow.Start;
    glm::vec2 end = arrow.End;
    float factor = 0.0f;
    if (start.x < end.x)
    {
        start = arrow.End;
        end = arrow.Start;
        factor = glm::pi<float>();
    }

    const glm::vec2 v = start - end;
    double angle = std::atan(v.y / v.x) + factor;

    float dist = glm::length(v) * 0.3f;

    constexpr float r = glm::radians(25.0f);

    lines.emplace_back(arrow.End, color);
    lines.emplace_back(glm::vec2{ arrow.End.x + std::cos(angle + r) * dist, arrow.End.y + std::sin(angle + r) * dist }, color);

    lines.emplace_back(arrow.End, color);
    lines.emplace_back(glm::vec2{ arrow.End.x + std::cos(angle - r) * dist, arrow.End.y + std::sin(angle - r) * dist }, color);
}

void VulkanLayer::DispatchNormal(const Layer& layer) const
{
    memcpy(m_NormalArrowsUniformBuffer.Map, &m_NormalArrows, sizeof(NormalArrows));

    VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(m_Device, m_CommandPool);

    Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, layer.Texture->GetMipLevels());

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_NormalPipelineLayout, 0, 1, &m_NormalDescriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_NormalPipeline);

    vkCmdPushConstants(commandBuffer, m_NormalPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::ivec2), &m_CanvasSize);

    const glm::uvec2 dispatchSize = glm::ceil(glm::vec2(m_CanvasSize) / 16.0f);
    vkCmdDispatch(commandBuffer, dispatchSize.x, dispatchSize.y, 1);

    Image::TransitionImageLayout(commandBuffer, layer.Texture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer.Texture->GetMipLevels());

    VK::EndSingleTimeCommands(m_Device, m_Application->GetQueue(), m_CommandPool, commandBuffer);
}

void VulkanLayer::ClearProject(bool clearLayers)
{
    if(clearLayers)
        m_LayerManager->ClearLayers(m_Device);

    m_GridRenderer->ClearLines();

    m_NormalArrowsRenderer->ClearLines();
    m_NormalArrows.Count = 0;

    if (m_SelectedLayer != -1)
        vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_NormalDescriptorSet);

    m_SelectedLayer = -1;
    m_SelectedNormalArrow = -1;
}

void VulkanLayer::SaveProject()
{
    std::ofstream out(m_CurrProject, std::ios::binary);

    out.write((char*)&m_CanvasSize.x, sizeof(glm::ivec2));

    out.write((char*)&m_NormalArrows.Count, sizeof(int));
    out.write((char*)m_NormalArrows.Arrows, sizeof(NormalArrow) * m_NormalArrows.Count);

    m_LayerManager->SaveLayers(out);

    out.close();
}

void VulkanLayer::LoadProject()
{
    std::ifstream in(m_CurrProject, std::ios::binary);

    in.read((char*)&m_CanvasSize.x, sizeof(glm::ivec2));

    in.read((char*)&m_NormalArrows.Count, sizeof(int));
    in.read((char*)m_NormalArrows.Arrows, sizeof(NormalArrow) * m_NormalArrows.Count);

    DrawNormalArrows();

    m_LayerManager->LoadLayers(in);

    in.close();

    BuildGrid();
}

void VulkanLayer::BuildGrid()
{
    std::vector<DebugRendererVertex> lines;
    lines.reserve(((size_t)m_CanvasSize.x + 1 + m_CanvasSize.y + 1 + 4) * 2);

    constexpr glm::vec3 color(1.0f);

    for (float x = 0; x <= m_CanvasSize.x; ++x)
    {
        lines.emplace_back(glm::vec2{ x, 0.0f }, color);
        lines.emplace_back(glm::vec2{ x, (float)m_CanvasSize.y }, color);
    }

    for (float y = 0; y <= m_CanvasSize.y; ++y)
    {
        lines.emplace_back(glm::vec2{ 0.0f, y }, color);
        lines.emplace_back(glm::vec2{ (float)m_CanvasSize.x, y }, color);
    }

    m_GridRenderer->AddLines(lines);
}

glm::vec2 VulkanLayer::GetMouseWorldPosition() const
{
    const glm::vec2 cameraExtension = m_Camera->GetOrthoExtension();

    // Mouse position to [0 - 1] to Camera extension * 2
    glm::vec2 point = Input::GetMousePosition() - m_Application->GetViewportWindowPos();
    point /= m_PrevViewportRegionAvail;
    point *= cameraExtension * 2.0f;

    // Adding word camera top left position
    const glm::vec3& cameraPos = m_Camera->GetPosition();
    point += glm::vec2{ -cameraPos.x, cameraPos.y } - cameraExtension;
    point.y *= -1;

    return point;
}
