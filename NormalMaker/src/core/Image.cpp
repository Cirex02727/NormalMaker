#include "vkpch.h"
#include "Image.h"

#include "Core.h"

void Image::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
    VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo
    {
        /* sType                 */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext                 */ nullptr,
        /* flags                 */ 0,
        /* imageType             */ VK_IMAGE_TYPE_2D,
        /* format                */ format,
        /* extent                */
        {
            /* width  */ static_cast<uint32_t>(width),
            /* height */ static_cast<uint32_t>(height),
            /* depth  */ 1
        },
        /* mipLevels             */ mipLevels,
        /* arrayLayers           */ 1,
        /* samples               */ numSample,
        /* tiling                */ tiling,
        /* usage                 */ usage,
        /* sharingMode           */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount */ 0,
        /* pQueueFamilyIndices   */ nullptr,
        /* initialLayout         */ VK_IMAGE_LAYOUT_UNDEFINED
    };

    VK(vkCreateImage(device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        /* pNext           */ nullptr,
        /* allocationSize  */ memRequirements.size,
        /* memoryTypeIndex */ VK::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };

    VK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

    VK(vkBindImageMemory(device, image, imageMemory, 0));
}

VkImageView Image::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        /* pNext            */ nullptr,
        /* flags            */ 0,
        /* image            */ image,
        /* viewType         */ VK_IMAGE_VIEW_TYPE_2D,
        /* format           */ format,
        /* components       */
        {
            /* r */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* g */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* b */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* a */ VK_COMPONENT_SWIZZLE_IDENTITY
        },
        /* subresourceRange */
        {
            /* aspectMask     */ aspectFlags,
            /* baseMipLevel   */ 0,
            /* levelCount     */ mipLevels,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        }
    };

    VkImageView imageView;
    VK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

    return imageView;
}

void Image::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
    uint32_t mipLevels)
{
    VkImageMemoryBarrier barrier
    {
        /* sType               */ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        /* pNext               */ nullptr,
        /* srcAccessMask       */ VK_ACCESS_NONE,
        /* dstAccessMask       */ VK_ACCESS_NONE,
        /* oldLayout           */ oldLayout,
        /* newLayout           */ newLayout,
        /* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* image               */ image,
        /* subresourceRange    */
        {
            /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* baseMipLevel   */ 0,
            /* levelCount     */ mipLevels,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        }
    };

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    // else
    // {
    //     printf("Unsupported layout transition!");
    //     return;
    // }


    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void Image::TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

    TransitionImageLayout(commandBuffer, image, format, oldLayout, newLayout, mipLevels);

    VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
}

void Image::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region
    {
        /* bufferOffset      */ 0,
        /* bufferRowLength   */ 0,
        /* bufferImageHeight */ 0,
        /* imageSubresource  */
        {
            /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* mipLevel       */ 0,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        },
        /* imageOffset       */
        {
            /* x */ 0,
            /* y */ 0,
            /* z */ 0
        },
        /* imageExtent       */
        {
            /* width  */ width,
            /* height */ height,
            /* depth  */ 1
        }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void Image::CopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, VkImageLayout imageLayout)
{
    VkBufferImageCopy region
    {
        /* bufferOffset      */ 0,
        /* bufferRowLength   */ 0,
        /* bufferImageHeight */ 0,
        /* imageSubresource  */
        {
            /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* mipLevel       */ 0,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        },
        /* imageOffset       */
        {
            /* x */ 0,
            /* y */ 0,
            /* z */ 0
        },
        /* imageExtent       */
        {
            /* width  */ width,
            /* height */ height,
            /* depth  */ 1
        }
    };

    vkCmdCopyImageToBuffer(commandBuffer, image, imageLayout, buffer, 1, &region);
}

void Image::GenerateMipmaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
    uint32_t mipLevels, VkImageLayout endLayout)
{
    // // Check if image format supports linear blitting
    // VkFormatProperties formatProperties;
    // vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
    // 
    // if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    // {
    //     printf("Texture image format does not support linear blitting!");
    //     return;
    // }


    VkImageMemoryBarrier barrier
    {
        /* sType               */ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        /* pNext               */ nullptr,
        /* srcAccessMask       */ 0,
        /* dstAccessMask       */ 0,
        /* oldLayout           */ VK_IMAGE_LAYOUT_UNDEFINED,
        /* newLayout           */ VK_IMAGE_LAYOUT_UNDEFINED,
        /* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
        /* image               */ image,
        /* subresourceRange    */
        {
            /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* baseMipLevel   */ 0,
            /* levelCount     */ 1,
            /* baseArrayLayer */ 0,
            /* layerCount     */ 1
        }
    };

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit
        {
            /* srcSubresource */
            {
                /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
                /* mipLevel       */ i - 1,
                /* baseArrayLayer */ 0,
                /* layerCount     */ 1
            },
            /* srcOffsets[2]  */
            {
                { 0, 0, 0 },
                { mipWidth, mipHeight, 1 }
            },
            /* dstSubresource */
            {
                /* aspectMask     */ VK_IMAGE_ASPECT_COLOR_BIT,
                /* mipLevel       */ i,
                /* baseArrayLayer */ 0,
                /* layerCount     */ 1
            },
            /* dstOffsets[2]  */
            {
                { 0, 0, 0 },
                { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }
            }
        };

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = endLayout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1)  mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = endLayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void Image::Barrier(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkFormat format,
    VkImageLayout imageLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

    TransitionImageLayout(commandBuffer, image, format, imageLayout, imageLayout, mipLevels);

    VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
}

unsigned char* Image::Read(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, VkImage image, VkFormat format,
    uint32_t width, uint32_t height, uint32_t miplevels, VkImageLayout oldImageLayout)
{
    uint32_t size = width * height * 4;
    unsigned char* buff = new unsigned char[size];

    MappedBuffer outBuffer = Buffer::CreateMappedBuffer(device, physicalDevice, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

    if(oldImageLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        Image::TransitionImageLayout(commandBuffer, image, format,
            oldImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, miplevels);

    Image::CopyImageToBuffer(commandBuffer, image, outBuffer.Buffer,
        width, height, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);

    memcpy(buff, outBuffer.Map, size);

    DeleteBuffer(device, outBuffer);

    return buff;
}
