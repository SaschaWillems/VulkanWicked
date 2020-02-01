/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "DescriptorPool.h"

DescriptorPool::DescriptorPool(VkDevice device) 
{
	this->device = device;
}

DescriptorPool::~DescriptorPool() 
{
	vkDestroyDescriptorPool(device, handle, nullptr);
}

void DescriptorPool::create()
{
	assert(poolSizes.size() > 0);
	assert(maxSets > 0);
	VkDescriptorPoolCreateInfo CI{};// = vks::initializers::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 5 * 10);
	CI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	CI.pPoolSizes = poolSizes.data();
	CI.maxSets = maxSets;
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &CI, nullptr, &handle));
}

void DescriptorPool::setMaxSets(uint32_t maxSets)
{
	this->maxSets = maxSets;
}

void DescriptorPool::addPoolSize(VkDescriptorType type, uint32_t descriptorCount) {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = type;
	poolSize.descriptorCount = descriptorCount;
	poolSizes.push_back(poolSize);
}