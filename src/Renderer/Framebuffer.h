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

class Framebuffer {
private:
	VkDevice device;
	int32_t width;
	int32_t height;
public:
	VkFramebuffer handle;
	Framebuffer(VkDevice device);
	~Framebuffer();
};
