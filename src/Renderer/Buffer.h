/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanTools.h"
#include "vk_mem_alloc.h"
#include "DescriptorSet.h"

class Buffer
{
public:
	VmaAllocator* vmaAllocator = nullptr;
	VkDevice device;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo descriptor;
	DescriptorSet* descriptorSet = nullptr;
	VkDeviceSize size = 0;
	VkDeviceSize alignment = 0;
	void* mapped = nullptr;
	VkBufferUsageFlags usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags;
	VmaAllocation vmaAllocation;
	VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void unmap();
	VkResult bind(VkDeviceSize offset = 0);
	void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void copyTo(void* data, VkDeviceSize size);
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void destroy();
};