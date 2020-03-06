/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include <string>
#include "vulkan/vulkan.h"
#include "Buffer.h"
#include "vk_mem_alloc.h"

class Device
{
private:
	VkCommandPool commandPool = VK_NULL_HANDLE;
	void* pNext = nullptr;
	std::vector<const char*> enabledExtensions;
public:
	VmaAllocator vmaAllocator;
	VkDevice handle;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features{};
	VkPhysicalDeviceFeatures enabledFeatures{};
	VkPhysicalDeviceMemoryProperties memoryProperties;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<std::string> supportedExtensions;
	struct
	{
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} queueFamilyIndices;
	Device(VkPhysicalDevice physicalDevice);
	~Device();
	uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr);
	uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags);
	VkResult createBuffer(VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, Buffer* buffer, VkDeviceSize size, void* data = nullptr);
	void copyBuffer(Buffer* src, Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion = nullptr);
	VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
	void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
	bool extensionSupported(std::string extension);
	void enableExtension(const char* extension);
	void setPNext(void* pNext);
	VkResult create(VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
};

