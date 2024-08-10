#include "vkpch.h"
#include "VulkanUtils.h"

VkCommandBuffer VK::BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo
    {
        /* sType              */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /* pNext              */ nullptr,
        /* commandPool        */ commandPool,
        /* level              */ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /* commandBufferCount */ 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext            */ nullptr,
        /* flags            */ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        /* pInheritanceInfo */ nullptr
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VK::EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo
    {
        /* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
        /* pNext                */ nullptr,
        /* waitSemaphoreCount   */ 0,
        /* pWaitSemaphores      */ nullptr,
        /* pWaitDstStageMask    */ nullptr,
        /* commandBufferCount   */ 1,
        /* pCommandBuffers      */ &commandBuffer,
        /* signalSemaphoreCount */ 0,
        /* pSignalSemaphores    */ nullptr
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VK::CreateCommandPool(VkDevice device, uint32_t queueFamily, VkCommandPool* commandPool)
{
    VkCommandPoolCreateInfo poolInfo
    {
        /* sType            */ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        /* pNext            */ nullptr,
        /* flags            */ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        /* queueFamilyIndex */ queueFamily
    };

    VK(vkCreateCommandPool(device, &poolInfo, nullptr, commandPool));
}

uint32_t VK::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    printf("Error to find suitable Memory Type!");
    return -1;
}
