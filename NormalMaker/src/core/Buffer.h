#pragma once

struct BufferData
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
};

struct MappedBuffer : public BufferData
{
	void* Map;
};

#define DeleteBuffer(device, buffer) {                 \
	vkFreeMemory(device, buffer.Memory, nullptr);      \
	vkDestroyBuffer(device, buffer.Buffer, nullptr); } \

#define DeleteMappedBuffer(device, buffer) {           \
	vkUnmapMemory(device, buffer.Memory);              \
	vkFreeMemory(device, buffer.Memory, nullptr);      \
	vkDestroyBuffer(device, buffer.Buffer, nullptr); } \

class Buffer
{
public:
	static BufferData CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags = 0);

	static BufferData CreateDataBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags = 0);
	
	static MappedBuffer CreateMappedDataBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags = 0);

	static MappedBuffer CreateMappedBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags = 0);

	static void UpdateMappedBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size, MappedBuffer& buffer);

	static void CopyBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size);

	static void Barrier(VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t size, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
};
