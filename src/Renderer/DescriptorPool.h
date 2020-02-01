/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"

class DescriptorPool {
private:
	VkDevice device;
	std::vector<VkDescriptorPoolSize> poolSizes;
	uint32_t maxSets;
public:
	VkDescriptorPool handle;
	DescriptorPool(VkDevice device);
	~DescriptorPool();
	void create();
	void setMaxSets(uint32_t maxSets);
	void addPoolSize(VkDescriptorType type, uint32_t descriptorCount);
};