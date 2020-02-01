/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"

class CommandPool {
private:
	VkDevice device;
	uint32_t queueFamilyIndex;
	VkCommandPoolCreateFlags flags;
public:
	VkCommandPool handle;
	CommandPool(VkDevice device);
	~CommandPool();
	void create();
	void setQueueFamilyIndex(uint32_t queueFamilyIndex);
	void setFlags(VkCommandPoolCreateFlags flags);
};