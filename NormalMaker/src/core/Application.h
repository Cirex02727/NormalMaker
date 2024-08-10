#pragma once

#include <vulkan/vulkan_core.h>

class Application
{
public:
	virtual VkDevice              GetDevice()               const { return nullptr;                }
	virtual VkPhysicalDevice      GetPhysicalDevice()       const { return nullptr;                }
	virtual VkQueue               GetQueue()                const { return nullptr;                }
	virtual VkCommandPool         GetCommandPool()          const { return nullptr;                }

	virtual const glm::uvec2&     GetViewportSize()         const { return EMPTY_UVEC2;            }

	virtual const glm::uvec2&     GetViewportRegionAvail()  const { return EMPTY_UVEC2;            }
	virtual bool                  IsViewportHovered()       const { return false;                  }
	virtual const glm::vec2&      GetViewportWindowPos()    const { return EMPTY_VEC2;             }
	virtual const glm::vec2&      GetViewportWindowSize()   const { return EMPTY_VEC2;             }

	virtual VkRenderPass          GetRenderPass()           const { return nullptr;                }

	virtual VkFormat              GetSwapChainImageFormat() const { return VK_FORMAT_UNDEFINED;    }

	virtual VkSampleCountFlagBits GetMSAASamples()          const { return VK_SAMPLE_COUNT_1_BIT;  }

	virtual VkImageView           GetDepthImageView()       const { return nullptr;                }

	virtual VkSampler             GetViewportImageSampler() const { return nullptr;                }

	virtual uint32_t              GetImageCount()           const { return 0;                      }

	virtual VkDescriptorPool      GetImGuiDescriptorPool()  const { return nullptr;                }

private:
	glm::vec2  EMPTY_VEC2 = {};
	glm::uvec2 EMPTY_UVEC2 = {};
};
