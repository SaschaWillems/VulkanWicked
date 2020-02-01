/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <array>
#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"

class RenderPass {
private:
	VkDevice device;
	int32_t width;
	int32_t height;
	VkFramebuffer framebuffer;
	std::vector<VkAttachmentDescription> attachmentDescriptions;
	std::vector<VkSubpassDependency> subpassDependencies;
	std::vector<VkSubpassDescription> subpassDescriptions;
	std::vector<VkClearValue> clearValues;
public:
	VkRenderPass handle;
	RenderPass(VkDevice device);
	~RenderPass();
	void create();
	VkRenderPassBeginInfo getBeginInfo();
	void setDimensions(int32_t width, int32_t height);
	void setFrameBuffer(VkFramebuffer framebuffer);
	void setColorClearValue(uint32_t index, std::array<float, 4> values);
	void setDepthStencilClearValue(uint32_t index, float depth, uint32_t stencil);
	void addAttachmentDescription(VkAttachmentDescription description);
	void addSubpassDependency(VkSubpassDependency dependency);
	void addSubpassDescription(VkSubpassDescription description);
};