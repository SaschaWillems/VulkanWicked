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
#include "DescriptorPool.h"

class DescriptorSet {
private:
	VkDevice device = VK_NULL_HANDLE;
	DescriptorPool *pool = nullptr;
	std::vector<VkDescriptorSetLayout> layouts;
	std::vector<VkWriteDescriptorSet> descriptors;
public:
	VkDescriptorSet handle;
	DescriptorSet(VkDevice device);
	~DescriptorSet();
	void create();
	void setPool(DescriptorPool* pool);
	void addLayout(VkDescriptorSetLayout layout);
	void addLayout(DescriptorSetLayout* layout);
	void addDescriptor(VkWriteDescriptorSet descriptor);
	void addDescriptor(uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount = 1);
	void addDescriptor(uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1);
};
