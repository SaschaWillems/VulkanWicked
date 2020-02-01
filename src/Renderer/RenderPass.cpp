/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "RenderPass.h"

RenderPass::RenderPass(VkDevice device) 
{
	this->device = device;
}

RenderPass::~RenderPass()
{
	vkDestroyRenderPass(device, handle, nullptr);
}

void RenderPass::create() 
{
	VkRenderPassCreateInfo CI{};
	CI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	CI.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	CI.pAttachments = attachmentDescriptions.data();
	CI.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
	CI.pSubpasses = subpassDescriptions.data();
	CI.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	CI.pDependencies = subpassDependencies.data();
	VK_CHECK_RESULT(vkCreateRenderPass(device, &CI, nullptr, &handle));
}

VkRenderPassBeginInfo RenderPass::getBeginInfo() 
{
	VkRenderPassBeginInfo beginInfo = vks::initializers::renderPassBeginInfo();
	beginInfo.renderPass = handle;
	beginInfo.renderArea.offset.x = 0;
	beginInfo.renderArea.offset.y = 0;
	beginInfo.framebuffer = framebuffer;
	beginInfo.renderArea.extent.width = width;
	beginInfo.renderArea.extent.height = height;
	beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	beginInfo.pClearValues = clearValues.data();
	return beginInfo;
}

void RenderPass::setDimensions(int32_t width, int32_t height) 
{
	this->height = height;
	this->width = width;
}

void RenderPass::setFrameBuffer(VkFramebuffer framebuffer) 
{
	this->framebuffer = framebuffer;
}

void RenderPass::setColorClearValue(uint32_t index, std::array<float, 4> values)
{
	if (index + 1 > clearValues.size()) {
		clearValues.resize(index + 1);
	}
	memcpy(clearValues[index].color.float32, values.data(), sizeof(float) * 4);
}

void RenderPass::setDepthStencilClearValue(uint32_t index, float depth, uint32_t stencil) 
{
	if (index + 1 > clearValues.size()) {
		clearValues.resize(index + 1);
	}
	clearValues[index].depthStencil.depth = depth;
	clearValues[index].depthStencil.stencil = stencil;
}

void RenderPass::addAttachmentDescription(VkAttachmentDescription description)
{
	attachmentDescriptions.push_back(description);
}

void RenderPass::addSubpassDependency(VkSubpassDependency dependency) 
{
	subpassDependencies.push_back(dependency);
}

void RenderPass::addSubpassDescription(VkSubpassDescription description) {
	subpassDescriptions.push_back(description);
}