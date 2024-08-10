#include "vkpch.h"
#include "Buffer.h"

#include "Core.h"

BufferData Buffer::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags)
{
    VkBufferCreateInfo bufferInfo
    {
        /* sType                 */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        /* pNext                 */ nullptr,
        /* flags                 */ 0,
        /* size                  */ size,
        /* usage                 */ usage,
        /* sharingMode           */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount */ 0,
    };

    VkBuffer buffer;
    VK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));


    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo
    {
        /* sType           */ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        /* pNext           */ nullptr,
        /* allocationSize  */ memRequirements.size,
        /* memoryTypeIndex */ VK::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };

    VkMemoryAllocateFlagsInfoKHR flagsInfo =
    {
        /* sType      */ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR,
        /* pNext      */ nullptr,
        /* flags      */ nextFlags,
        /* deviceMask */ 0,
    };

    if (nextFlags)
        allocInfo.pNext = &flagsInfo;

    VkDeviceMemory bufferMemory;
    VK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

    VK(vkBindBufferMemory(device, buffer, bufferMemory, 0));

    return BufferData{ buffer, bufferMemory };
}

BufferData Buffer::CreateDataBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags)
{
    BufferData stagingBuffer = CreateBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mappedData;
    VK(vkMapMemory(device, stagingBuffer.Memory, 0, size, 0, &mappedData));
    memcpy(mappedData, data, size);
    vkUnmapMemory(device, stagingBuffer.Memory);

    BufferData buffer = CreateBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, properties, nextFlags);

    CopyBuffer(device, queue, commandPool, stagingBuffer.Buffer, buffer.Buffer, size);

    vkDestroyBuffer(device, stagingBuffer.Buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.Memory, nullptr);

    return buffer;
}

MappedBuffer Buffer::CreateMappedDataBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags)
{
    BufferData stagingBuffer = CreateBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mappedData;
    VK(vkMapMemory(device, stagingBuffer.Memory, 0, size, 0, &mappedData));
    memcpy(mappedData, data, static_cast<size_t>(size));

    BufferData buffer = CreateBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, properties, nextFlags);

    CopyBuffer(device, queue, commandPool, stagingBuffer.Buffer, buffer.Buffer, size);

    vkDestroyBuffer(device, stagingBuffer.Buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.Memory, nullptr);

    return MappedBuffer{ buffer, mappedData };
}

MappedBuffer Buffer::CreateMappedBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags)
{
    BufferData buffer = CreateBuffer(device, physicalDevice, size, usage, properties, nextFlags);

    void* mappedData;
    VK(vkMapMemory(device, buffer.Memory, 0, size, 0, &mappedData));

    return MappedBuffer{ buffer, mappedData };
}

void Buffer::UpdateMappedBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, MappedBuffer& buffer)
{
    BufferData stagingBuffer = CreateBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mappedData;
    VK(vkMapMemory(device, stagingBuffer.Memory, 0, size, 0, &mappedData));
    memcpy(mappedData, data, static_cast<size_t>(size));

    CopyBuffer(device, queue, commandPool, stagingBuffer.Buffer, buffer.Buffer, size);

    vkDestroyBuffer(device, stagingBuffer.Buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.Memory, nullptr);
}

void Buffer::CopyBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size)
{
    VkCommandBuffer commandBuffer = VK::BeginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion
    {
        /* srcOffset */ 0,
        /* dstOffset */ 0,
        /* size      */ size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VK::EndSingleTimeCommands(device, queue, commandPool, commandBuffer);
}
