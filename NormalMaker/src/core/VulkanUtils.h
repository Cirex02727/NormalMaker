#pragma once

namespace VK
{
    // Single Time Commands
    VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

    void EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

    // Create Objects
    void CreateCommandPool(VkDevice device, uint32_t queueFamily, VkCommandPool* commandPool);

    // Others
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}
