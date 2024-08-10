#include "vkpch.h"
#include "Texture.h"

#include "Core.h"

Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, unsigned char* pixels,
    int width, int height, VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout)
    : m_Width(width), m_Height(height), m_Format(format)
{
    Create(device, physicalDevice, queue, commandPool, pixels, format, flags, mipmap, endLayout);
}

Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
    const std::string& filePath, VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout)
    : m_Format(format)
{
    int texChannels;
    stbi_uc* pixels = stbi_load((filePath).c_str(), &m_Width, &m_Height, &texChannels, STBI_rgb_alpha);
    
    Create(device, physicalDevice, queue, commandPool, pixels, format, flags, mipmap, endLayout);

    stbi_image_free(pixels);
}

Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
    int width, int height, VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout)
    : m_Width(width), m_Height(height), m_Format(format)
{
    uint32_t imageSize = m_Width * m_Height * 4;
    unsigned char* pixels = new unsigned char[imageSize];
    memset(pixels, 0, imageSize);

    Create(device, physicalDevice, queue, commandPool, pixels, format, flags, mipmap, endLayout);

    delete[] pixels;
}

void Texture::Delete(VkDevice device) const
{
    if (m_Sampler)
        vkDestroySampler(device, m_Sampler, nullptr);

    vkDestroyImageView(device, m_View, nullptr);

    vkFreeMemory(device, m_Memory, nullptr);
    vkDestroyImage(device, m_Image, nullptr);
}

void Texture::Create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, unsigned char* pixels,
    VkFormat format, VkImageUsageFlags flags, bool mipmap, VkImageLayout endLayout)
{
    VkDeviceSize imageSize = (VkDeviceSize)m_Width * m_Height * 4;

    if (!pixels)
    {
        printf("Failed to load texture image!\n");
        return;
    }

    if (mipmap)
        m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;

    BufferData stagingBuffer = Buffer::CreateBuffer(device, physicalDevice, static_cast<uint32_t>(imageSize),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    VK(vkMapMemory(device, stagingBuffer.Memory, 0, imageSize, 0, &data));
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.Memory);

    Image::CreateImage(device, physicalDevice, m_Width, m_Height, m_MipLevels, VK_SAMPLE_COUNT_1_BIT, format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | flags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_Memory);

    // Transfer buffer data to image
    {
        VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

        Image::TransitionImageLayout(commandBuffer, m_Image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
        Image::CopyBufferToImage(commandBuffer, stagingBuffer.Buffer, m_Image, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));

        VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
    }

    DeleteBuffer(device, stagingBuffer);

    // Create mipmap or change image layout
    {
        VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

        if (mipmap)
            Image::GenerateMipmaps(commandBuffer, m_Image, format, m_Width, m_Height, m_MipLevels, endLayout);
        else
            Image::TransitionImageLayout(commandBuffer, m_Image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, endLayout, 0);

        VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
    }


    CreateView(device);
}

void Texture::CreateView(VkDevice device)
{
    m_View = Image::CreateImageView(device, m_Image, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
}

void Texture::CreateSampler(VkDevice device, VkPhysicalDevice physicalDevice, VkFilter magFilter, VkFilter minFilter)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo
    {
        /* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        /* pNext                   */ nullptr,
        /* flags                   */ 0,
        /* magFilter               */ magFilter,
        /* minFilter               */ minFilter,
        /* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
        /* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* mipLodBias              */ 0.0f,
        /* anisotropyEnable        */ VK_TRUE,
        /* maxAnisotropy           */ properties.limits.maxSamplerAnisotropy,
        /* compareEnable           */ VK_FALSE,
        /* compareOp               */ VK_COMPARE_OP_ALWAYS,
        /* minLod                  */ 0.0f,
        /* maxLod                  */ static_cast<float>(m_MipLevels),
        /* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        /* unnormalizedCoordinates */ VK_FALSE
    };

    VK(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));
}

void Texture::TransferLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier
    {
        /* sType               */ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        /* pNext               */ nullptr,
        /* srcAccessMask       */ VK_ACCESS_TRANSFER_READ_BIT,
        /* dstAccessMask       */ VK_ACCESS_MEMORY_READ_BIT,
        /* oldLayout           */ oldLayout,
        /* newLayout           */ newLayout,
        /* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* image               */ m_Image,
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
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}
