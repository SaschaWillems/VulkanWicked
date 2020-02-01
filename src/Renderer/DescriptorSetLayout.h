/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"

class DescriptorSetLayout {
private:
	VkDevice device;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
public:
	VkDescriptorSetLayout handle = VK_NULL_HANDLE;
	DescriptorSetLayout(VkDevice device);
	~DescriptorSetLayout();
	void create();
	void addBinding(VkDescriptorSetLayoutBinding binding);
	void addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);
};