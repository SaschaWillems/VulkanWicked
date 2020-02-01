/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "DescriptorSet.h"

DescriptorSet::DescriptorSet(VkDevice device) 
{
	this->device = device;
}

DescriptorSet::~DescriptorSet() 
{
	// @todo
}

void DescriptorSet::create() 
{
	VkDescriptorSetAllocateInfo descriptorSetAI = vks::initializers::descriptorSetAllocateInfo(pool->handle, layouts.data(), static_cast<uint32_t>(layouts.size()));
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAI, &handle));
	for (auto& descriptor : descriptors) {
		descriptor.dstSet = handle;
	}
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);
}

void DescriptorSet::setPool(DescriptorPool* pool) 
{
	this->pool = pool;
}

void DescriptorSet::addLayout(VkDescriptorSetLayout layout) 
{
	layouts.push_back(layout);
}

void DescriptorSet::addLayout(DescriptorSetLayout* layout) 
{
	layouts.push_back(layout->handle);
}

void DescriptorSet::addDescriptor(VkWriteDescriptorSet descriptor) 
{
	descriptors.push_back(descriptor);
}

void DescriptorSet::addDescriptor(uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount) 
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	descriptors.push_back(writeDescriptorSet);
}

void DescriptorSet::addDescriptor(uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount) 
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pImageInfo = imageInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	descriptors.push_back(writeDescriptorSet);
}
