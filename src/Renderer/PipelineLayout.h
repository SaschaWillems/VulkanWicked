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
#include "DescriptorSetLayout.h"

class PipelineLayout {
private:
	VkDevice device = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> layouts;
	std::vector<VkPushConstantRange> pushConstantRanges;
public:
	VkPipelineLayout handle;
	PipelineLayout(VkDevice device);
	~PipelineLayout();
	void create();
	void addLayout(VkDescriptorSetLayout layout);
	void addLayout(DescriptorSetLayout* layout);
	void addPushConstantRange(uint32_t size, uint32_t offset, VkShaderStageFlags stageFlags);
	VkPushConstantRange getPushConstantRange(uint32_t index);
};