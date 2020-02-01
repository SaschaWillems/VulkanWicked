/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "DescriptorSetLayout.h"

DescriptorSetLayout::DescriptorSetLayout(VkDevice device) 
{
	this->device = device;
}

DescriptorSetLayout::~DescriptorSetLayout() 
{
	vkDestroyDescriptorSetLayout(device, handle, nullptr);
}

void DescriptorSetLayout::create() 
{
	VkDescriptorSetLayoutCreateInfo CI = vks::initializers::descriptorSetLayoutCreateInfo(bindings.data(), static_cast<uint32_t>(bindings.size()));
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &CI, nullptr, &handle));
}

void DescriptorSetLayout::addBinding(VkDescriptorSetLayoutBinding binding) 
{
	bindings.push_back(binding);
}

void DescriptorSetLayout::addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount) 
{
	VkDescriptorSetLayoutBinding setLayoutBinding{};
	setLayoutBinding.descriptorType = type;
	setLayoutBinding.stageFlags = stageFlags;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorCount = descriptorCount;
	bindings.push_back(setLayoutBinding);
}