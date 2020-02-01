/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "CommandPool.h"

CommandPool::CommandPool(VkDevice device)
{
	this->device = device;
}

CommandPool::~CommandPool()
{
	vkDestroyCommandPool(device, handle, nullptr);
}

void CommandPool::create() {
	VkCommandPoolCreateInfo CI{};
	CI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CI.queueFamilyIndex = queueFamilyIndex;
	CI.flags = flags;
	VK_CHECK_RESULT(vkCreateCommandPool(device, &CI, nullptr, &handle));
}

void CommandPool::setQueueFamilyIndex(uint32_t queueFamilyIndex) {
	this->queueFamilyIndex = queueFamilyIndex;
}

void CommandPool::setFlags(VkCommandPoolCreateFlags flags) {
	this->flags = flags;
}