#pragma once

class Texture
{
public:
	Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, unsigned char* pixels, int width, int height,
		VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, const std::string& filePath,
		VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, int width, int height,
		VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void Delete(VkDevice device) const;

	void CreateSampler(VkDevice device, VkPhysicalDevice physicalDevice, VkFilter magFilter, VkFilter minFilter);

	void TransferLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

	int GetWidth()  const { return m_Width;  }
	int GetHeight() const { return m_Height; }

	glm::ivec2 GetSize() const { return { m_Width, m_Height }; }

	uint32_t GetMipLevels() const { return m_MipLevels; }

	VkFormat GetFormat() const { return m_Format; }

	VkImage     GetImage()   const { return m_Image;   }
	VkImageView GetView()    const { return m_View;    }
	VkSampler   GetSampler() const { return m_Sampler; }

private:
	void Create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, unsigned char* pixels,
		VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void CreateView(VkDevice device);

private:
	int            m_Width     = 0;
	int            m_Height    = 0;

	uint32_t       m_MipLevels = 0;

	VkFormat       m_Format    = VK_FORMAT_UNDEFINED;

	VkImage        m_Image     = nullptr;
	VkDeviceMemory m_Memory    = nullptr;

	VkImageView    m_View      = nullptr;

	VkSampler      m_Sampler   = nullptr;
};
