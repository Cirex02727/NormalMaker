#include "vkpch.h"
#include "VulkanApplication.h"

#include "Core.h"

const std::vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,

#ifdef VK_DEBUG
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#endif
};

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

#define VSYNC true
#define MAX_FPS 60.0f

VulkanApplication::VulkanApplication()
    : m_ViewportSize(WIDTH, HEIGHT)
{
    SaveManager::Load();
}

bool VulkanApplication::Init()
{
    if (glfwInit() != GLFW_TRUE)
    {
        printf("Error init GLFW!\n");
        return false;
    }

    if (glfwVulkanSupported() != GLFW_TRUE)
    {
        printf("Error Vulkan support!\n");
        return false;
    }

    if (!Window::CreateGLFWWindow(m_ViewportSize.x, m_ViewportSize.y, "Vulkan"))
        return false;

    Window::SetWindowUserPointer(this);
    Window::SetFramebufferSizeCallback(FramebufferResizeCallback);

    Input::Init();

    InitVulkan();

    for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
    {
        layer->Init(*this);
        layer->OnResize(m_ViewportSize);
    }

    return true;
}

void VulkanApplication::InitVulkan()
{
    // Setup Vulkan
    CreateInstance();      // Create Vulkan Instace
    SetupDebugMessenger(); // Attach debug layer if needed

    CreateSurface(); // Create Window Surface

    PickPhysicalDevice();  // Pick a GPU and select graphycs queues families
    CreateLogicalDevice(); // Create Render Queues

    CreateSwapChain();  // Swapchain
    CreateImageViews(); // 

    CreateRenderPass();         // RenderPasses
    CreateViewportRenderPass(); // 

    VK::CreateCommandPool(m_Device, m_QueueFamilyIndices.graphicsFamily, &m_ClearCommandPool);    // CommandPools
    VK::CreateCommandPool(m_Device, m_QueueFamilyIndices.graphicsFamily, &m_ViewportCommandPool); // 

    CreateViewportImage();      // Viewport Attachments
    CreateViewportImageViews(); // 

    CreateColorResources();       // 
    CreateDepthResources();       // Framebuffers
    CreateClearFramebuffers();    // 
    CreateViewportFramebuffers(); // 

    CreateCommandBuffers();         // Commandbuffers
    CreateViewportCommandBuffers(); // 

    CreateSyncObjects(); // SyncObjects

    CreateImGuiDescriptorPool();            // 
    CreateImGuiRenderPass();                // 
    VK::CreateCommandPool(m_Device, m_QueueFamilyIndices.graphicsFamily, &m_ImGuiCommandPool); // ImGui
    CreateImGuiCommandBuffers();            // 
    CreateImGuiFramebuffers();              // 
    InitImGui();                            // 

    CreateViewportImageSampler();  // Viewport ImGui Images
    CreateViewportDescriptorSet(); // 


    CreateResetAlphaDescriptorPool();
    CreateResetAlphaDescriptorSetLayout();
    CreateResetAlphaDescriptorSets();
    Shader::CreateComputePipeline(m_Device, "resetalpha.comp",
        { m_ResetAlphaDescriptorSetLayout }, {}, m_ResetAlphaPipelineLayout, m_ResetAlphaPipeline);
}

// --------------------- Run ---------------------//

void VulkanApplication::Run()
{
    float deltaTime = 0.0f, lastFrameTime = 0.0f, maxDeltaTime = 1.0f / MAX_FPS;

    ImGuiIO& io = ImGui::GetIO();

    float updateAs = 0.0f;

    while (!Window::WindowShouldClose() && m_IsRunning)
    {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrameTime;

        if (deltaTime < maxDeltaTime)
            continue;

        lastFrameTime = time;

        Input::Update();

        glfwPollEvents();

        if (m_ViewportSize.x == 0 || m_ViewportSize.y == 0 || m_FramebufferResized)
        {
            m_FramebufferResized = false;

            int sizeX = m_ViewportSize.x;
            int sizeY = m_ViewportSize.y;
            Window::GetFramebufferSize(&sizeX, &sizeY);
            m_ViewportSize.x = sizeX;
            m_ViewportSize.y = sizeY;

            continue;
        }

        if (m_ViewportSize.x != m_SwapChainExtent.width ||
            m_ViewportSize.y != m_SwapChainExtent.height)
        {
            RecreateSwapChain();
            continue;
        }

        // Update //
        {
            Timer tm;

            for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
                layer->OnUpdate(deltaTime);

            updateAs = tm.ElapsedMillis();
        }

        // ImGui //
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Docking space
        {
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace Demo", nullptr, window_flags);
            ImGui::PopStyleVar();

            ImGui::PopStyleVar(2);

            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }

            if (ImGui::BeginMenuBar())
            {
                for (auto& layer : m_LayerStack)
                    layer->OnImGuiRenderMenuBar(m_IsRunning);

                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        // ImGui Windows
        ImGui::Begin("Settings");
        {
            ImGui::Text("FPS: %.1f (%.2f ms)", deltaTime * 1000.0f, 1.0f / deltaTime);

            ImGui::Text("As Update: %.1f ms", updateAs);

            Window::WindowState windowState = Window::GetWindowState();
            if (ImGui::BeginCombo("Window State", Window::WindowStateToString(windowState)))
            {
                bool selected = windowState == Window::WindowState::Windowed;
                if (ImGui::Selectable("Windowed", selected))
                    Window::SetWindowState(Window::WindowState::Windowed);

                if (selected)
                    ImGui::SetItemDefaultFocus();

                selected = windowState == Window::WindowState::Fullscreen;
                if (ImGui::Selectable("Fullscreen", selected))
                    Window::SetWindowState(Window::WindowState::Fullscreen);

                if (selected)
                    ImGui::SetItemDefaultFocus();

                selected = windowState == Window::WindowState::WindowedBorderless;
                if (ImGui::Selectable("WindowedBorderless", selected))
                    Window::SetWindowState(Window::WindowState::WindowedBorderless);

                if (selected)
                    ImGui::SetItemDefaultFocus();


                ImGui::EndCombo();
            }
        }
        ImGui::End();

        for (auto& layer : m_LayerStack)
            layer->OnImGuiRender();


        uint32_t frameIndex;
        if (!AquireFrame(frameIndex))
        {
            ImGui::EndFrame();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            continue;
        }


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
        ImGui::Begin("Viewport");
        {
            ImVec2 freeSpace = ImGui::GetContentRegionAvail();

            m_ViewportRegionAvail = { static_cast<uint32_t>(freeSpace.x), static_cast<uint32_t>(freeSpace.y) };

            m_ViewportHovered = ImGui::IsWindowHovered();

            ImVec2 windowPos = ImGui::GetCurrentContext()->CurrentWindow->ContentRegionRect.Min;
            m_ViewportWindowPos = glm::vec2{ windowPos.x, windowPos.y } - glm::vec2(Window::GetWindowPos());

            ImVec2 pos = ImGui::GetCursorScreenPos();

            ImGui::Image(m_ViewportDescriptorSets[frameIndex], freeSpace);
        }
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }


        DrawFrame(frameIndex);
        PresentFrame(frameIndex);
    }

    VK(vkDeviceWaitIdle(m_Device));
}

bool VulkanApplication::AquireFrame(uint32_t& frameIndex)
{
    VK(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &frameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || result == VK_NOT_READY || m_FramebufferResized)
    {
        m_FramebufferResized = false;
        RecreateSwapChain();
        return false;
    }
    VK(result);

    return true;
}

void VulkanApplication::DrawFrame(uint32_t frameIndex)
{
    VK(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]));

    // Clear Swap chain framebuffer
    RecordClearCommandBuffer(m_ClearCommandBuffers[m_CurrentFrame], frameIndex);

    // Render Viewport
    RecordViewportCommandBuffer(m_ViewportCommandBuffers[m_CurrentFrame], frameIndex);

    // Render ImGui
    RecordImGuiCommandBuffer(m_ImGuiCommandBuffers[m_CurrentFrame], frameIndex);

    std::array<VkCommandBuffer, 3> submitCommandBuffers = { m_ClearCommandBuffers[m_CurrentFrame], m_ViewportCommandBuffers[m_CurrentFrame], m_ImGuiCommandBuffers[m_CurrentFrame] };
    
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo
    {
        /* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
        /* pNext                */ nullptr,
        /* waitSemaphoreCount   */ 1,
        /* pWaitSemaphores      */ &m_ImageAvailableSemaphores[m_CurrentFrame],
        /* pWaitDstStageMask    */ waitStages,
        /* commandBufferCount   */ static_cast<uint32_t>(submitCommandBuffers.size()),
        /* pCommandBuffers      */ submitCommandBuffers.data(),
        /* signalSemaphoreCount */ 1,
        /* pSignalSemaphores    */ &m_RenderFinishedSemaphores[m_CurrentFrame]
    };

    VK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]));
}

void VulkanApplication::PresentFrame(uint32_t frameIndex)
{
    VkPresentInfoKHR presentInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        /* pNext              */ nullptr,
        /* waitSemaphoreCount */ 1,
        /* pWaitSemaphores    */ &m_RenderFinishedSemaphores[m_CurrentFrame],
        /* swapchainCount     */ 1,
        /* pSwapchains        */ &m_SwapChain,
        /* pImageIndices      */ &frameIndex,
        /* pResults           */ nullptr
    };

    VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
    {
        m_FramebufferResized = false;
        RecreateSwapChain();
    }
    else VK(result);


    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

#pragma region Record Commandbuffers

void VulkanApplication::RecordClearCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const
{
    VkCommandBufferBeginInfo beginInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext            */ nullptr,
        /* flags            */ 0,
        /* pInheritanceInfo */ nullptr
    };

    VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    std::array<VkClearValue, 1> clearValues =
    {
        VkClearValue
        {
            /* color        */ { 0.1f, 0.1f, 0.1f, 1.0f },
        }
    };

    VkRenderPassBeginInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        /* pNext           */ nullptr,
        /* renderPass      */ m_ClearRenderPass,
        /* framebuffer     */ m_ClearFramebuffers[imageIndex],
        /* renderArea      */
        {
            /* offset */ { 0, 0 },
            /* extent */ m_SwapChainExtent
        },
        /* clearValueCount */ static_cast<uint32_t>(clearValues.size()),
        /* pClearValues    */ clearValues.data()
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);

    VK(vkEndCommandBuffer(commandBuffer));
}

void VulkanApplication::RecordViewportCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext            */ nullptr,
        /* flags            */ 0,
        /* pInheritanceInfo */ nullptr
    };

    VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
        layer->OnPreRender(commandBuffer, imageIndex);


    std::array<VkClearValue, 2> clearValues =
    {
        VkClearValue
        {
            /* color        */ { 0.1f, 0.1f, 0.1f, 1.0f },
        },
        VkClearValue
        {
            /* depthStencil */ { 1.0f, 0U }
        }
    };

    VkRenderPassBeginInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        /* pNext           */ nullptr,
        /* renderPass      */ m_ViewportRenderPass,
        /* framebuffer     */ m_ViewportFramebuffers[imageIndex],
        /* renderArea      */
        {
            /* offset */ { 0, 0 },
            /* extent */ m_SwapChainExtent
        },
        /* clearValueCount */ static_cast<uint32_t>(clearValues.size()),
        /* pClearValues    */ clearValues.data()
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        VkViewport viewport
        {
            /* x        */ 0.0f,
            /* y        */ 0.0f,
            /* width    */ (float)m_SwapChainExtent.width,
            /* height   */ (float)m_SwapChainExtent.height,
            /* minDepth */ 0.0f,
            /* maxDepth */ 1.0f
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor
        {
            /* offset */ { 0, 0 },
            /* extent */ m_SwapChainExtent
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


        for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
            layer->OnRender(commandBuffer, imageIndex);
    }
    vkCmdEndRenderPass(commandBuffer);

    for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
        layer->OnPostRender(commandBuffer, imageIndex);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResetAlphaPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResetAlphaPipelineLayout, 0, 1, &m_ResetAlphaDescriptorSets[imageIndex], 0, nullptr);

    vkCmdDispatch(commandBuffer, (m_SwapChainExtent.width + 15) / 16, (m_SwapChainExtent.height + 15) / 16, 1);

    VkImageMemoryBarrier barrier =
    {
        /* sType               */ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        /* pNext               */ nullptr,
        /* srcAccessMask       */ VK_ACCESS_NONE,
        /* dstAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
        /* oldLayout           */ VK_IMAGE_LAYOUT_GENERAL,
        /* newLayout           */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        /* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* image               */ m_ViewportImages[imageIndex],
        /* subresourceRange    */
        {
            /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* baseMipLevel   */ 0,
            /* levelCount     */ 1,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        }
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    VK(vkEndCommandBuffer(commandBuffer));
}

void VulkanApplication::RecordImGuiCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const
{
    VkCommandBufferBeginInfo beginInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext            */ nullptr,
        /* flags            */ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        /* pInheritanceInfo */ nullptr
    };

    VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));


    std::array<VkClearValue, 1> clearValues =
    {
        VkClearValue
        {
            /* color */ { 0.1f, 0.1f, 0.1f, 1.0f },
        }
    };

    VkRenderPassBeginInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        /* pNext           */ nullptr,
        /* renderPass      */ m_ImGuiRenderPass,
        /* framebuffer     */ m_ImGuiFramebuffers[imageIndex],
        /* renderArea      */
        {
            /* offset */ { 0, 0 },
            /* extent */ m_SwapChainExtent
        },
        /* clearValueCount */ static_cast<uint32_t>(clearValues.size()),
        /* pClearValues    */ clearValues.data()
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    VK(vkEndCommandBuffer(commandBuffer));
}

#pragma endregion

// --------------------- Initialization ---------------------//

#pragma region Create Instance

void VulkanApplication::CreateInstance()
{
#ifdef VK_DEBUG
    if (enableValidationLayers && !CheckValidationLayerSupport())
    {
        printf("Error Validation Layer!\n");
        return;
    }
#endif

    std::vector<const char*> extensions;
    if (!CheckExtensions(extensions))
    {
        printf("Error Extensions available");
        return;
    }

    
    VkApplicationInfo appInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* pNext              */ nullptr,
        /* pApplicationName   */ "Vulkan Test",
        /* applicationVersion */ VK_MAKE_VERSION(1, 0, 0),
        /* pEngineName        */ "No Engine",
        /* engineVersion      */ VK_MAKE_VERSION(1, 0, 0),
        /* apiVersion         */ VK_API_VERSION_1_0
    };

    VkValidationFeatureEnableEXT feature
    {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
    };
    VkValidationFeaturesEXT features
    {
        /* sType                          */ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        /* pNext                          */ nullptr,
        /* enabledValidationFeatureCount  */ 1,
        /* pEnabledValidationFeatures     */ &feature,
        /* disabledValidationFeatureCount */ 0,
        /* pDisabledValidationFeatures    */ nullptr
    };

    VkInstanceCreateInfo createInfo
    {
        /* sType                   */ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        /* pNext                   */ &features,
        /* flags                   */ 0,
        /* pApplicationInfo        */ &appInfo,
        /* enabledLayerCount       */ 0,
        /* ppEnabledLayerNames     */ nullptr,
        /* enabledExtensionCount   */ (unsigned int)extensions.size(),
        /* ppEnabledExtensionNames */ extensions.data()
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = (unsigned int)validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }

    VK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
}

bool VulkanApplication::CheckValidationLayerSupport() const
{
    uint32_t layerCount;
    VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));


    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }

        if (!layerFound)
            return false;
    }

    return true;
}

bool VulkanApplication::CheckExtensions(std::vector<const char*>& glfwExtensions)
{
    uint32_t extensionCount = 0;
    VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> extensions(extensionCount);

    VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));

    printf("Available extensions:\n");
    for (const VkExtensionProperties& extension : extensions)
        printf("\t%s\n", extension.extensionName);


    bool allExtensionsIncluded = true;
    glfwExtensions = GetRequiredExtensions();

    for (const char* glfwExtension : glfwExtensions)
    {
        bool extensionIncluded = false;
        for (const auto& extension : extensions)
            if (strcmp(extension.extensionName, glfwExtension) == 0)
                extensionIncluded = true;

        if (!extensionIncluded)
            return false;
    }

    return allExtensionsIncluded;
}

std::vector<const char*> VulkanApplication::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

#pragma endregion

void VulkanApplication::SetupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    VK(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
}

void VulkanApplication::CreateSurface()
{
    VK(glfwCreateWindowSurface(m_Instance, Window::GetWindow(), nullptr, &m_Surface));
}

#pragma region PickPhysicalDevice

void VulkanApplication::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        printf("Error no GPU with Vulkan supoort!");
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    m_PhysicalDevice = VK_NULL_HANDLE;

    for (const VkPhysicalDevice& device : devices)
        if (IsDeviceSuitable(device))
        {
            m_PhysicalDevice = device;
            m_MSAASamples = GetMaxUsableSampleCount();
            break;
        }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        printf("Error to find suitable GPU!");
        return;
    }
}

bool VulkanApplication::IsDeviceSuitable(const VkPhysicalDevice& device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    if (!CheckDeviceExtensionSupport(device))
        return false;

    QuerySwapChainSupport(device);
    if (m_SwapChainSupport.formats.empty() || m_SwapChainSupport.presentModes.empty())
        return false;

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        supportedFeatures.geometryShader && supportedFeatures.samplerAnisotropy &&
        FindQueueFamilies(device);
}

bool VulkanApplication::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

void VulkanApplication::QuerySwapChainSupport(const VkPhysicalDevice& device)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &m_SwapChainSupport.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        m_SwapChainSupport.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, m_SwapChainSupport.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        m_SwapChainSupport.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, m_SwapChainSupport.presentModes.data());
    }
}

bool VulkanApplication::FindQueueFamilies(const VkPhysicalDevice& device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    bool hasGraphics = false, hasPresent = false;

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            m_QueueFamilyIndices.graphicsFamily = i;
            hasGraphics = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
        if (presentSupport)
        {
            m_QueueFamilyIndices.presentFamily = i;
            hasPresent = true;
        }

        if (hasGraphics && hasPresent)
            return true;

        ++i;
    }
    
    return false;
}

VkSampleCountFlagBits VulkanApplication::GetMaxUsableSampleCount() const
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

#pragma endregion

void VulkanApplication::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<unsigned int> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily, m_QueueFamilyIndices.presentFamily };

    float queuePriority = 1.0f;
    for (unsigned int queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.emplace_back(
            /* sType            */ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            /* pNext            */ nullptr,
            /* flags            */ 0,
            /* queueFamilyIndex */ queueFamily,
            /* queueCount       */ 1,
            /* pQueuePriorities */ &queuePriority
        );
    }


    VkPhysicalDeviceFeatures deviceFeatures =
    {
        .sampleRateShading = VK_TRUE,
        .wideLines = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    VkDeviceCreateInfo createInfo
    {
        /* sType                   */ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        /* pNext                   */ nullptr,
        /* flags                   */ 0,
        /* queueCreateInfoCount    */ (unsigned int)queueCreateInfos.size(),
        /* pQueueCreateInfos       */ queueCreateInfos.data(),
        /* enabledLayerCount       */ 0,
        /* ppEnabledLayerNames     */ nullptr,
        /* enabledExtensionCount   */ (unsigned int)deviceExtensions.size(),
        /* ppEnabledExtensionNames */ deviceExtensions.data(),
        /* pEnabledFeatures        */ &deviceFeatures
    };

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = (unsigned int)validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    VK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));

    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.presentFamily,  0, &m_PresentQueue);
}

#pragma region SwapChain

void VulkanApplication::CreateSwapChain()
{
    QuerySwapChainSupport(m_PhysicalDevice); // For Resize support

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_SwapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(m_SwapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(m_SwapChainSupport.capabilities);

    m_ImageCount = m_SwapChainSupport.capabilities.minImageCount + 1;
    if (m_SwapChainSupport.capabilities.maxImageCount > 0 && m_ImageCount > m_SwapChainSupport.capabilities.maxImageCount)
        m_ImageCount = m_SwapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo
    {
        /* sType                 */ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        /* pNext                 */ nullptr,
        /* flags                 */ 0,
        /* surface               */ m_Surface,
        /* minImageCount         */ m_ImageCount,
        /* imageFormat           */ surfaceFormat.format,
        /* imageColorSpace       */ surfaceFormat.colorSpace,
        /* imageExtent           */ extent,
        /* imageArrayLayers      */ 1,
        /* imageUsage            */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* imageSharingMode      */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount */ 0,
        /* pQueueFamilyIndices   */ nullptr,
        /* preTransform          */ m_SwapChainSupport.capabilities.currentTransform,
        /* compositeAlpha        */ VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        /* presentMode           */ presentMode,
        /* clipped               */ VK_TRUE,
        /* oldSwapchain          */ VK_NULL_HANDLE
    };

    if (m_SwapChainSupport.capabilities.supportedTransforms & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (m_SwapChainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    

    uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.graphicsFamily, m_QueueFamilyIndices.presentFamily };
    
    if (m_QueueFamilyIndices.graphicsFamily != m_QueueFamilyIndices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    
    VK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));
    
    
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_ImageCount, nullptr);
    m_SwapChainImages.resize(m_ImageCount);
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_ImageCount, m_SwapChainImages.data());
    
    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
}

void VulkanApplication::CreateImageViews()
{
    m_SwapChainImageViews.resize(m_ImageCount);
    for (size_t i = 0; i < m_ImageCount; i++)
        m_SwapChainImageViews[i] = Image::CreateImageView(m_Device, m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

VkSurfaceFormatKHR VulkanApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // VK_FORMAT_B8G8R8A8_SRGB
    const std::array<VkFormat, 4> requestSurfaceImageFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    
    for (const VkFormat& format : requestSurfaceImageFormat)
        for (const auto& availableFormat : availableFormats)
            if (format == availableFormat.format && availableFormat.colorSpace == requestSurfaceColorSpace)
                return availableFormat;
    
    return availableFormats[0];
}

VkPresentModeKHR VulkanApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
#if VSYNC
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#endif

    for(const VkPresentModeKHR& mode : present_modes)
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
            if (mode == availablePresentMode)
                return availablePresentMode;
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    Window::GetFramebufferSize(&width, &height);

    VkExtent2D actualExtent =
    {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width  = glm::clamp(actualExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
    actualExtent.height = glm::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

#pragma endregion

#pragma region RenderPasses

void VulkanApplication::CreateRenderPass()
{
    std::array<VkAttachmentDescription, 1> attachments =
    {
        VkAttachmentDescription
        {
            /* flags          */ 0,
            /* format         */ m_SwapChainImageFormat,
            /* samples        */ VK_SAMPLE_COUNT_1_BIT,
            /* loadOp         */ VK_ATTACHMENT_LOAD_OP_CLEAR,
            /* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
            /* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
            /* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
            /* finalLayout    */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };
    
    VkAttachmentReference colorAttachmentRef
    {
        /* attachment */ 0,
        /* layout     */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass
    {
        /* flags                   */ 0,
        /* pipelineBindPoint       */ VK_PIPELINE_BIND_POINT_GRAPHICS,
        /* inputAttachmentCount    */ 0,
        /* pInputAttachments       */ nullptr,
        /* colorAttachmentCount    */ 1,
        /* pColorAttachments       */ &colorAttachmentRef,
        /* pResolveAttachments     */ nullptr,
        /* pDepthStencilAttachment */ nullptr,
        /* preserveAttachmentCount */ 0,
        /* pPreserveAttachments    */ nullptr
    };

    std::array<VkSubpassDependency, 2> dependencies =
    {
        VkSubpassDependency
        {
            /* srcSubpass      */ VK_SUBPASS_EXTERNAL,
            /* dstSubpass      */ 0,
            /* srcStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            /* dstStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* srcAccessMask   */ VK_ACCESS_MEMORY_READ_BIT,
            /* dstAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            /* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
        },
        VkSubpassDependency
        {
            /* srcSubpass      */ 0,
            /* dstSubpass      */ VK_SUBPASS_EXTERNAL,
            /* srcStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* dstStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            /* srcAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            /* dstAccessMask   */ VK_ACCESS_MEMORY_READ_BIT,
            /* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
        }
    };

    VkRenderPassCreateInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        /* pNext           */ nullptr,
        /* flags           */ 0,
        /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
        /* pAttachments    */ attachments.data(),
        /* subpassCount    */ 1,
        /* pSubpasses      */ &subpass,
        /* dependencyCount */ static_cast<uint32_t>(dependencies.size()),
        /* pDependencies   */ dependencies.data()
    };

    VK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_ClearRenderPass));
}

void VulkanApplication::CreateViewportRenderPass()
{
    std::array<VkAttachmentDescription, 3> attachments =
    {
        VkAttachmentDescription
        {
            /* flags          */ 0,
            /* format         */ m_SwapChainImageFormat,
            /* samples        */ m_MSAASamples,
            /* loadOp         */ VK_ATTACHMENT_LOAD_OP_CLEAR,
            /* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
            /* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
            /* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
            /* finalLayout    */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        VkAttachmentDescription
        {
            /* flags          */ 0,
            /* format         */ FindDepthFormat(),
            /* samples        */ m_MSAASamples,
            /* loadOp         */ VK_ATTACHMENT_LOAD_OP_CLEAR,
            /* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
            /* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
            /* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
            /* finalLayout    */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
        VkAttachmentDescription
        {
            /* flags          */ 0,
            /* format         */ m_SwapChainImageFormat,
            /* samples        */ VK_SAMPLE_COUNT_1_BIT,
            /* loadOp         */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
            /* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
            /* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
            /* finalLayout    */ VK_IMAGE_LAYOUT_GENERAL
        }
    };

    VkAttachmentReference colorAttachmentRef
    {
        /* attachment */ 0,
        /* layout     */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef
    {
        /* attachment */ 1,
        /* layout     */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorAttachmentResolveRef
    {
        /* attachment */ 2,
        /* layout     */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass
    {
        /* flags                   */ 0,
        /* pipelineBindPoint       */ VK_PIPELINE_BIND_POINT_GRAPHICS,
        /* inputAttachmentCount    */ 0,
        /* pInputAttachments       */ nullptr,
        /* colorAttachmentCount    */ 1,
        /* pColorAttachments       */ &colorAttachmentRef,
        /* pResolveAttachments     */ &colorAttachmentResolveRef,
        /* pDepthStencilAttachment */ &depthAttachmentRef,
        /* preserveAttachmentCount */ 0,
        /* pPreserveAttachments    */ nullptr
    };

    std::array<VkSubpassDependency, 2> dependencies =
    {
        VkSubpassDependency
        {
            /* srcSubpass      */ VK_SUBPASS_EXTERNAL,
            /* dstSubpass      */ 0,
            /* srcStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            /* dstStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* srcAccessMask   */ VK_ACCESS_MEMORY_READ_BIT,
            /* dstAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            /* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
        },
        VkSubpassDependency
        {
            /* srcSubpass      */ 0,
            /* dstSubpass      */ VK_SUBPASS_EXTERNAL,
            /* srcStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* dstStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            /* srcAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            /* dstAccessMask   */ VK_ACCESS_MEMORY_READ_BIT,
            /* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
        }
    };

    VkRenderPassCreateInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        /* pNext           */ nullptr,
        /* flags           */ 0,
        /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
        /* pAttachments    */ attachments.data(),
        /* subpassCount    */ 1,
        /* pSubpasses      */ &subpass,
        /* dependencyCount */ static_cast<uint32_t>(dependencies.size()),
        /* pDependencies   */ dependencies.data()
    };

    VK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_ViewportRenderPass));
}

#pragma endregion

void VulkanApplication::CreateViewportImage()
{
    m_ViewportImages.resize(m_ImageCount);
    m_ViewportImagesMemory.resize(m_ImageCount);

    for (uint32_t i = 0; i < m_ImageCount; i++)
    {
        Image::CreateImage(m_Device, m_PhysicalDevice, m_SwapChainExtent.width, m_SwapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_SwapChainImageFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ViewportImages[i], m_ViewportImagesMemory[i]);

        Image::TransitionImageLayout(m_Device, m_GraphicsQueue, m_ViewportCommandPool, m_ViewportImages[i], m_SwapChainImageFormat,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }
}

void VulkanApplication::CreateViewportImageViews()
{
    m_ViewportImageViews.resize(m_ImageCount);
    for (size_t i = 0; i < m_ImageCount; i++)
        m_ViewportImageViews[i] = Image::CreateImageView(m_Device, m_ViewportImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

#pragma region Framebuffers

void VulkanApplication::CreateColorResources()
{
    Image::CreateImage(m_Device, m_PhysicalDevice, m_SwapChainExtent.width, m_SwapChainExtent.height, 1, m_MSAASamples, m_SwapChainImageFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ColorImage, m_ColorImageMemory);
    m_ColorImageView = Image::CreateImageView(m_Device, m_ColorImage, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    Image::TransitionImageLayout(m_Device, m_GraphicsQueue, m_ViewportCommandPool, m_ColorImage, m_SwapChainImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
}

void VulkanApplication::CreateDepthResources()
{
    VkFormat depthFormat = FindDepthFormat();

    Image::CreateImage(m_Device, m_PhysicalDevice, m_SwapChainExtent.width, m_SwapChainExtent.height, 1, m_MSAASamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = Image::CreateImageView(m_Device, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
        imageAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

    Image::TransitionImageLayout(m_Device, m_GraphicsQueue, m_ViewportCommandPool, m_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void VulkanApplication::CreateClearFramebuffers()
{
    m_ClearFramebuffers.resize(m_ImageCount);

    for (size_t i = 0; i < m_ImageCount; i++)
    {
        std::array<VkImageView, 1> attachments =
        {
            m_SwapChainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo
        {
            /* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            /* pNext           */ nullptr,
            /* flags           */ 0,
            /* renderPass      */ m_ClearRenderPass,
            /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
            /* pAttachments    */ attachments.data(),
            /* width           */ m_SwapChainExtent.width,
            /* height          */ m_SwapChainExtent.height,
            /* layers          */ 1
        };

        VK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_ClearFramebuffers[i]));
    }
}

void VulkanApplication::CreateViewportFramebuffers()
{
    m_ViewportFramebuffers.resize(m_ImageCount);

    for (size_t i = 0; i < m_ImageCount; i++)
    {
        std::array<VkImageView, 3> attachments =
        {
            m_ColorImageView,
            m_DepthImageView,
            m_ViewportImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo
        {
            /* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            /* pNext           */ nullptr,
            /* flags           */ 0,
            /* renderPass      */ m_ViewportRenderPass,
            /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
            /* pAttachments    */ attachments.data(),
            /* width           */ m_SwapChainExtent.width,
            /* height          */ m_SwapChainExtent.height,
            /* layers          */ 1
        };

        VK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_ViewportFramebuffers[i]));
    }
}

#pragma endregion

#pragma region CommandBuffers

void VulkanApplication::CreateCommandBuffers()
{
    m_ClearCommandBuffers.resize(m_ImageCount);

    VkCommandBufferAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* commandPool        */ m_ClearCommandPool,
        /* level              */ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /* commandBufferCount */ m_ImageCount
    };

    VK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_ClearCommandBuffers.data()));
}

void VulkanApplication::CreateViewportCommandBuffers()
{
    m_ViewportCommandBuffers.resize(m_ImageCount);

    VkCommandBufferAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* commandPool        */ m_ViewportCommandPool,
        /* level              */ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /* commandBufferCount */ m_ImageCount
    };

    VK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_ViewportCommandBuffers.data()));
}

#pragma endregion

void VulkanApplication::CreateSyncObjects()
{
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo
    {
        /* sType */ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        /* pNext */ nullptr,
        /* flags */ 0
    };

    VkFenceCreateInfo fenceInfo
    {
        /* sType */ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        /* pNext */ nullptr,
        /* flags */ VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
        VK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
        VK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]));
    }
}

#pragma region ImGui

void VulkanApplication::CreateImGuiDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 11> poolSizes =
    {
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_SAMPLER,                /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, /* descriptorCount */ 1000 },
        VkDescriptorPoolSize{ /* type */ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       /* descriptorCount */ 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo
    {
        /* sType         */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext         */ nullptr,
        /* flags         */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        /* maxSets       */ 1000,
        /* poolSizeCount */ static_cast<uint32_t>(poolSizes.size()),
        /* pPoolSizes    */ poolSizes.data()
    };

    VK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_ImGuiDescriptorPool));
}

void VulkanApplication::CreateImGuiRenderPass()
{
    std::array<VkAttachmentDescription, 1> attachments =
    {
        VkAttachmentDescription
        {
            /* flags          */ 0,
            /* format         */ m_SwapChainImageFormat,
            /* samples        */ VK_SAMPLE_COUNT_1_BIT,
            /* loadOp         */ VK_ATTACHMENT_LOAD_OP_LOAD,
            /* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
            /* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            /* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
            /* initialLayout  */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            /* finalLayout    */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }
    };

    VkAttachmentReference colorAttachmentRef
    {
        /* attachment */ 0,
        /* layout     */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass
    {
        /* flags                   */ 0,
        /* pipelineBindPoint       */ VK_PIPELINE_BIND_POINT_GRAPHICS,
        /* inputAttachmentCount    */ 0,
        /* pInputAttachments       */ nullptr,
        /* colorAttachmentCount    */ 1,
        /* pColorAttachments       */ &colorAttachmentRef,
        /* pResolveAttachments     */ nullptr,
        /* pDepthStencilAttachment */ nullptr,
        /* preserveAttachmentCount */ 0,
        /* pPreserveAttachments    */ nullptr
    };

    std::array<VkSubpassDependency, 1> dependencies =
    {
        VkSubpassDependency
        {
            /* srcSubpass      */ VK_SUBPASS_EXTERNAL,
            /* dstSubpass      */ 0,
            /* srcStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* dstStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            /* srcAccessMask   */ 0,
            /* dstAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            /* dependencyFlags */ 0
        }
    };

    VkRenderPassCreateInfo renderPassInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        /* pNext           */ nullptr,
        /* flags           */ 0,
        /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
        /* pAttachments    */ attachments.data(),
        /* subpassCount    */ 1,
        /* pSubpasses      */ &subpass,
        /* dependencyCount */ static_cast<uint32_t>(dependencies.size()),
        /* pDependencies   */ dependencies.data()
    };

    VK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_ImGuiRenderPass));
}

void VulkanApplication::CreateImGuiCommandBuffers()
{
    m_ImGuiCommandBuffers.resize(m_ImageCount);

    VkCommandBufferAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* commandPool        */ m_ImGuiCommandPool,
        /* level              */ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /* commandBufferCount */ m_ImageCount
    };

    VK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_ImGuiCommandBuffers.data()));
}

void VulkanApplication::CreateImGuiFramebuffers()
{
    m_ImGuiFramebuffers.resize(m_ImageCount);

    for (size_t i = 0; i < m_ImageCount; i++)
    {
        std::array<VkImageView, 1> attachments =
        {
            m_SwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo
        {
            /* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            /* pNext           */ nullptr,
            /* flags           */ 0,
            /* renderPass      */ m_ImGuiRenderPass,
            /* attachmentCount */ static_cast<uint32_t>(attachments.size()),
            /* pAttachments    */ attachments.data(),
            /* width           */ m_SwapChainExtent.width,
            /* height          */ m_SwapChainExtent.height,
            /* layers          */ 1
        };

        VK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_ImGuiFramebuffers[i]));
    }
}

void VulkanApplication::InitImGui() const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard // Enable Keyboard Controls
        | ImGuiConfigFlags_NavEnableGamepad              // Enable Gamepad Controls
        | ImGuiConfigFlags_DockingEnable
        | ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // ------------------- Set Dark Theme ------------------- //
    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    ImGui_ImplGlfw_InitForVulkan(Window::GetWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info =
    {
        /* Instance                         */ m_Instance,
        /* PhysicalDevice                   */ m_PhysicalDevice,
        /* Device                           */ m_Device,
        /* QueueFamily                      */ m_QueueFamilyIndices.graphicsFamily,
        /* Queue                            */ m_GraphicsQueue,
        /* PipelineCache                    */ nullptr,
        /* DescriptorPool                   */ m_ImGuiDescriptorPool,
        /* Subpass                          */ 0,
        /* MinImageCount                    */ m_ImageCount,
        /* ImageCount                       */ m_ImageCount,
        /* MSAASamples                      */ VK_SAMPLE_COUNT_1_BIT,
        /* Allocator                        */ nullptr,
        /* (*CheckVkResultFn)(VkResult err) */ check_vk_result
    };

    ImGui_ImplVulkan_Init(&init_info, m_ImGuiRenderPass);

    // Upload Fonts
    {
        VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(m_Device, m_ViewportCommandPool);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        VK::EndSingleTimeCommands(m_Device, m_GraphicsQueue, m_ViewportCommandPool, commandBuffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

#pragma endregion

void VulkanApplication::CreateViewportImageSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo
    {
        /* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        /* pNext                   */ nullptr,
        /* flags                   */ 0,
        /* magFilter               */ VK_FILTER_LINEAR,
        /* minFilter               */ VK_FILTER_LINEAR,
        /* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
        /* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* mipLodBias              */ 0.0f,
        /* anisotropyEnable        */ VK_TRUE,
        /* maxAnisotropy           */ properties.limits.maxSamplerAnisotropy,
        /* compareEnable           */ VK_FALSE,
        /* compareOp               */ VK_COMPARE_OP_ALWAYS,
        /* minLod                  */ -1000.0f,
        /* maxLod                  */  1000.0f,
        /* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        /* unnormalizedCoordinates */ VK_FALSE
    };

    VK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_ViewportImageSampler));
}

void VulkanApplication::CreateViewportDescriptorSet()
{
    m_ViewportDescriptorSets.resize(m_ImageCount);
    for (uint32_t i = 0; i < m_ImageCount; i++)
        m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_ViewportImageSampler, m_ViewportImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

#pragma region ResetAlpha

void VulkanApplication::CreateResetAlphaDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        VkDescriptorPoolSize
        {
            /* type            */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount */ m_ImageCount
        }
    };

    VkDescriptorPoolCreateInfo poolInfo
    {
        /* sType         */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext         */ nullptr,
        /* flags         */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        /* maxSets       */ m_ImageCount,
        /* poolSizeCount */ static_cast<uint32_t>(poolSizes.size()),
        /* pPoolSizes    */ poolSizes.data()
    };

    VK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_ResetAlphaDescriptorPool));
}

void VulkanApplication::CreateResetAlphaDescriptorSetLayout()
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

    VK(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_ResetAlphaDescriptorSetLayout));
}

void VulkanApplication::CreateResetAlphaDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(m_ImageCount, m_ResetAlphaDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* descriptorPool     */ m_ResetAlphaDescriptorPool,
        /* descriptorSetCount */ m_ImageCount,
        /* pSetLayouts        */ layouts.data()
    };

    m_ResetAlphaDescriptorSets.resize(m_ImageCount);
    VK(vkAllocateDescriptorSets(m_Device, &allocInfo, m_ResetAlphaDescriptorSets.data()));

    for (size_t i = 0; i < m_ImageCount; ++i)
    {
        VkDescriptorImageInfo viewportInfo
        {
            /* sampler     */ m_ViewportImageSampler,
            /* imageView   */ m_ViewportImageViews[i],
            /* imageLayout */ VK_IMAGE_LAYOUT_GENERAL
        };

        std::vector<VkWriteDescriptorSet> descriptorWrites =
        {
            VkWriteDescriptorSet
            {
                /* sType            */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                /* pNext            */ nullptr,
                /* dstSet           */ m_ResetAlphaDescriptorSets[i],
                /* dstBinding       */ 0,
                /* dstArrayElement  */ 0,
                /* descriptorCount  */ 1,
                /* descriptorType   */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                /* pImageInfo       */ &viewportInfo,
                /* pBufferInfo      */ nullptr,
                /* pTexelBufferView */ nullptr
            }
        };

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

#pragma endregion

// --------------------- Const Functions ---------------------//

VkFormat VulkanApplication::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    printf("Failed to find supported format!");
    return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanApplication::FindDepthFormat() const
{
    return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// --------------------- Destroy ---------------------//

void VulkanApplication::RecreateSwapChain()
{
    int sizeX = m_ViewportSize.x;
    int sizeY = m_ViewportSize.y;
    Window::GetFramebufferSize(&sizeX, &sizeY);
    m_ViewportSize.x = sizeX;
    m_ViewportSize.y = sizeY;

    if (m_ViewportSize.x == 0 || m_ViewportSize.y == 0)
        return;

    VK(vkDeviceWaitIdle(m_Device));

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();

    CreateViewportImage();
    CreateViewportImageViews();

    CreateColorResources();
    CreateDepthResources();
    CreateClearFramebuffers();
    CreateViewportFramebuffers();

    CreateImGuiFramebuffers();

    CreateViewportDescriptorSet();
    CreateResetAlphaDescriptorSets();

    for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
        layer->OnResize(m_ViewportSize);
}

void VulkanApplication::CleanupSwapChain()
{
    // ---------------------- Destroy Framebuffers --------------------- //
    vkDestroyImageView(m_Device, m_ColorImageView, nullptr);
    vkDestroyImage(m_Device, m_ColorImage, nullptr);
    vkFreeMemory(m_Device, m_ColorImageMemory, nullptr);

    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

    for (size_t i = 0; i < m_ImageCount; i++)
        vkDestroyFramebuffer(m_Device, m_ClearFramebuffers[i], nullptr);

    for (size_t i = 0; i < m_ImageCount; i++)
        vkDestroyFramebuffer(m_Device, m_ViewportFramebuffers[i], nullptr);

    for (size_t i = 0; i < m_ImageCount; i++)
        vkDestroyFramebuffer(m_Device, m_ImGuiFramebuffers[i], nullptr);


    // --------------------- Destroy ImageViews --------------------- //
    for (size_t i = 0; i < m_ImageCount; i++)
        vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);

    for (size_t i = 0; i < m_ImageCount; i++)
        vkDestroyImageView(m_Device, m_ViewportImageViews[i], nullptr);


    // --------------------- Destroy Images -------------------- //
    for (size_t i = 0; i < m_ImageCount; i++)
    {
        vkDestroyImage(m_Device, m_ViewportImages[i], nullptr);
        vkFreeMemory(m_Device, m_ViewportImagesMemory[i], nullptr);
    }


    vkFreeDescriptorSets(m_Device, m_ImGuiDescriptorPool, static_cast<uint32_t>(m_ViewportDescriptorSets.size()), m_ViewportDescriptorSets.data());
    vkFreeDescriptorSets(m_Device, m_ResetAlphaDescriptorPool, static_cast<uint32_t>(m_ResetAlphaDescriptorSets.size()), m_ResetAlphaDescriptorSets.data());

    vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
}

void VulkanApplication::Destroy()
{
    for (std::shared_ptr<ApplicationLayer>& layer : m_LayerStack)
        layer->Destroy();

    m_LayerStack.clear();

    CleanupSwapChain();

    vkDestroySampler(m_Device, m_ViewportImageSampler, nullptr);


    vkDestroyPipeline(m_Device, m_ResetAlphaPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_ResetAlphaPipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(m_Device, m_ResetAlphaDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_Device, m_ResetAlphaDescriptorPool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkFreeCommandBuffers(m_Device, m_ImGuiCommandPool, static_cast<uint32_t>(m_ImGuiCommandBuffers.size()), m_ImGuiCommandBuffers.data());
    vkDestroyCommandPool(m_Device, m_ImGuiCommandPool, nullptr);
    vkDestroyRenderPass(m_Device, m_ImGuiRenderPass, nullptr);
    vkDestroyDescriptorPool(m_Device, m_ImGuiDescriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
    }

    vkFreeCommandBuffers(m_Device, m_ViewportCommandPool, static_cast<uint32_t>(m_ViewportCommandBuffers.size()), m_ViewportCommandBuffers.data());
    vkFreeCommandBuffers(m_Device, m_ClearCommandPool, static_cast<uint32_t>(m_ClearCommandBuffers.size()), m_ClearCommandBuffers.data());

    vkDestroyCommandPool(m_Device, m_ViewportCommandPool, nullptr);
    vkDestroyCommandPool(m_Device, m_ClearCommandPool, nullptr);

    vkDestroyRenderPass(m_Device, m_ViewportRenderPass, nullptr);
    vkDestroyRenderPass(m_Device, m_ClearRenderPass, nullptr);

    vkDestroyDevice(m_Device, nullptr);

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

    vkDestroyInstance(m_Instance, nullptr);

    Window::DestroyWindow();

    glfwTerminate();

    SaveManager::Unload();
}

// --------------------- Static Functions ---------------------//

void VulkanApplication::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = (VulkanApplication*)glfwGetWindowUserPointer(window);
    app->m_FramebufferResized = true;
}
