#pragma once

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
	unsigned int graphicsFamily;
	unsigned int presentFamily;
};

class VulkanApplication : public Application
{
public:
	static const int MAX_FRAMES_IN_FLIGHT = 2;

public:
	VulkanApplication();

	bool Init();

	void Run();

	void Destroy();

public:
	template<typename T>
	void PushLayer()
	{
		static_assert(std::is_base_of<ApplicationLayer, T>::value, "Pushed type is not subclass of Layer!");
		m_LayerStack.emplace_back(std::make_shared<T>());
	}

	void PushLayer(const std::shared_ptr<ApplicationLayer>& layer) { m_LayerStack.emplace_back(layer); }

public:
	virtual VkDevice              GetDevice()               const { return m_Device;               }
	virtual VkPhysicalDevice      GetPhysicalDevice()       const { return m_PhysicalDevice;       }
	virtual VkQueue               GetQueue()                const { return m_GraphicsQueue;        }
	virtual VkCommandPool         GetCommandPool()          const { return m_ViewportCommandPool;  }

	virtual const glm::uvec2&     GetViewportSize()         const { return m_ViewportSize;         }

	virtual const glm::uvec2&     GetViewportRegionAvail()  const { return m_ViewportRegionAvail;  }
	virtual bool                  IsViewportHovered()       const { return m_ViewportHovered;      }
	virtual const glm::vec2&      GetViewportWindowPos()    const { return m_ViewportWindowPos;    }

	virtual VkRenderPass          GetRenderPass()           const { return m_ViewportRenderPass;   }

	virtual VkFormat              GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }

	virtual VkSampleCountFlagBits GetMSAASamples()          const { return m_MSAASamples;          }

	virtual VkImageView           GetDepthImageView()       const { return m_DepthImageView;       }

	virtual VkSampler             GetViewportImageSampler() const { return m_ViewportImageSampler; }

	virtual uint32_t              GetImageCount()           const { return m_ImageCount;           }

	virtual VkDescriptorPool      GetImGuiDescriptorPool()  const { return m_ImGuiDescriptorPool;  }

private:
	bool AquireFrame(uint32_t& frameIndex);
	void DrawFrame(uint32_t frameIndex);
	void PresentFrame(uint32_t frameIndex);

	void RecordClearCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
	void RecordViewportCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);
	void RecordImGuiCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;

private:
	void InitVulkan();

	void CreateInstance();
	bool CheckValidationLayerSupport() const;
	bool CheckExtensions(std::vector<const char*>& glfwExtensions);
	std::vector<const char*> GetRequiredExtensions() const;

	void SetupDebugMessenger();

	void CreateSurface();

	void PickPhysicalDevice();
	bool IsDeviceSuitable(const VkPhysicalDevice& device);
	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const;
	void QuerySwapChainSupport(const VkPhysicalDevice& device);
	bool FindQueueFamilies(const VkPhysicalDevice& device);
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;

	void CreateLogicalDevice();

	void CreateSwapChain();
	void CreateImageViews();
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void CreateRenderPass();
	void CreateViewportRenderPass();

	void CreateViewportImage();
	void CreateViewportImageViews();

	void CreateColorResources();
	void CreateDepthResources();
	void CreateClearFramebuffers();
	void CreateViewportFramebuffers();

	void CreateCommandBuffers();
	void CreateViewportCommandBuffers();

	void CreateSyncObjects();

	void CreateImGuiDescriptorPool();
	void CreateImGuiRenderPass();
	void CreateImGuiCommandBuffers();
	void CreateImGuiFramebuffers();
	void InitImGui() const;

	void CreateViewportImageSampler();
	void CreateViewportDescriptorSet();

	void CreateResetAlphaDescriptorPool();
	void CreateResetAlphaDescriptorSetLayout();
	void CreateResetAlphaDescriptorSets();

private:
	void RecreateSwapChain();
	void CleanupSwapChain();

private:
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	VkFormat FindDepthFormat() const;

private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
	glm::uvec2  m_ViewportSize = {};

	std::vector<std::shared_ptr<ApplicationLayer>> m_LayerStack;

	// ---------------------------- Device ---------------------------- //
	VkInstance               m_Instance           = nullptr;
	VkSurfaceKHR             m_Surface            = nullptr;
	VkDebugUtilsMessengerEXT m_DebugMessenger     = nullptr;
	VkPhysicalDevice         m_PhysicalDevice     = nullptr;

	QueueFamilyIndices       m_QueueFamilyIndices = {};
	SwapChainSupportDetails  m_SwapChainSupport   = {};

	uint32_t                 m_ImageCount         = MAX_FRAMES_IN_FLIGHT;

	VkSampleCountFlagBits    m_MSAASamples        = VK_SAMPLE_COUNT_1_BIT;

	VkDevice                 m_Device             = nullptr;
	VkQueue                  m_GraphicsQueue      = nullptr;
	VkQueue                  m_PresentQueue       = nullptr;
	

	VkSwapchainKHR           m_SwapChain            = nullptr;			   // 
	VkFormat                 m_SwapChainImageFormat = VK_FORMAT_UNDEFINED; // 
	VkExtent2D               m_SwapChainExtent      = {};                  // Swapchain
														                   // 
	std::vector<VkImage>     m_SwapChainImages      = {};                  // 
	std::vector<VkImageView> m_SwapChainImageViews  = {};                  // 


	// ------------------------ Clear ------------------------ //
	VkRenderPass                 m_ClearRenderPass     = nullptr;
	VkCommandPool                m_ClearCommandPool    = nullptr;

	std::vector<VkFramebuffer>   m_ClearFramebuffers   = {};

	std::vector<VkCommandBuffer> m_ClearCommandBuffers = {};


	// ------------------------ Viewport ------------------------ //
	VkRenderPass                 m_ViewportRenderPass            = nullptr;
	VkCommandPool                m_ViewportCommandPool           = nullptr;

	std::vector<VkImage>         m_ViewportImages                = {};
	std::vector<VkDeviceMemory>  m_ViewportImagesMemory          = {};
	std::vector<VkImageView>     m_ViewportImageViews            = {};

	VkImage                      m_ColorImage                    = nullptr; // 
	VkDeviceMemory               m_ColorImageMemory              = nullptr; // 
	VkImageView                  m_ColorImageView                = nullptr; // 
	                                                                        // Framebuffer
	VkImage                      m_DepthImage                    = nullptr; // 
	VkDeviceMemory               m_DepthImageMemory              = nullptr; // 
	VkImageView                  m_DepthImageView                = nullptr; // 

	std::vector<VkFramebuffer>   m_ViewportFramebuffers          = {};

	std::vector<VkCommandBuffer> m_ViewportCommandBuffers        = {};

	VkSampler                    m_ViewportImageSampler          = nullptr;

	std::vector<VkDescriptorSet> m_ViewportDescriptorSets        = {};

	VkDescriptorPool             m_ResetAlphaDescriptorPool      = nullptr;

	VkDescriptorSetLayout        m_ResetAlphaDescriptorSetLayout = nullptr;
	std::vector<VkDescriptorSet> m_ResetAlphaDescriptorSets      = {};

	VkPipelineLayout             m_ResetAlphaPipelineLayout      = nullptr;
	VkPipeline                   m_ResetAlphaPipeline            = nullptr;


	// ------------------------ ImGui ------------------------ //
	VkRenderPass                 m_ImGuiRenderPass     = nullptr;
	VkCommandPool                m_ImGuiCommandPool    = nullptr;

	VkDescriptorPool             m_ImGuiDescriptorPool = nullptr;

	std::vector<VkFramebuffer>   m_ImGuiFramebuffers   = {};

	std::vector<VkCommandBuffer> m_ImGuiCommandBuffers = {};


	glm::uvec2     m_ViewportRegionAvail = {};
	bool           m_ViewportHovered     = false;
	glm::vec2      m_ViewportWindowPos   = {};

	std::vector<VkSemaphore> m_ImageAvailableSemaphores = {};
	std::vector<VkSemaphore> m_RenderFinishedSemaphores = {};
	std::vector<VkFence>     m_InFlightFences           = {};

	bool         m_IsRunning          = true;
	bool         m_FramebufferResized = false;
	unsigned int m_CurrentFrame       = 0;
};
