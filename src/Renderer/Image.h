/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"
#include "Device.h"

class Image
{
private:
	Device* device;
	VkDeviceMemory memory;
	VkImageType type;
	VkFormat format;
	VkExtent3D extent;
	uint32_t mipLevels = 1;
	uint32_t arrayLayers = 1;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
public:
	VkImage handle;
	Image(Device* device);
	~Image();
	void create();
	void setType(VkImageType type);
	void setFormat(VkFormat format);
	void setExtent(VkExtent3D extent);
	void setNumMipLevels(uint32_t mipLevels);
	void setNumArrayLayers(uint32_t arrayLayers);
	void setSampleCount(VkSampleCountFlagBits samples);
	void setTiling(VkImageTiling tiling);
	void setUsage(VkImageUsageFlags usage);
	void setSharingMode(VkSharingMode sharingMode);
};