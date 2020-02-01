/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "PipelineLayout.h"

PipelineLayout::PipelineLayout(VkDevice device)
{
	this->device = device;
}

PipelineLayout::~PipelineLayout()
{
	vkDestroyPipelineLayout(device, handle, nullptr);
}

void PipelineLayout::create()
{
	VkPipelineLayoutCreateInfo CI = vks::initializers::pipelineLayoutCreateInfo(layouts.data(), static_cast<uint32_t>(layouts.size()));
	CI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	CI.pPushConstantRanges = pushConstantRanges.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &CI, nullptr, &handle));
}

void PipelineLayout::addLayout(VkDescriptorSetLayout layout)
{
	layouts.push_back(layout);
}

void PipelineLayout::addLayout(DescriptorSetLayout* layout)
{
	layouts.push_back(layout->handle);
}

void PipelineLayout::addPushConstantRange(uint32_t size, uint32_t offset, VkShaderStageFlags stageFlags)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = stageFlags;
	pushConstantRange.offset = offset;
	pushConstantRange.size = size;
	pushConstantRanges.push_back(pushConstantRange);
}

VkPushConstantRange PipelineLayout::getPushConstantRange(uint32_t index)
{
	assert(index < pushConstantRanges.size());
	return pushConstantRanges[index];
}