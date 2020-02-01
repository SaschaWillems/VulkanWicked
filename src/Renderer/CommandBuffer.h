/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Buffer.h"

class CommandBuffer {
private:
	VkDevice device;
	CommandPool *pool = nullptr;
	VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
public:
	VkCommandBuffer handle;
	CommandBuffer(VkDevice device);
	~CommandBuffer();
	void create();
	void setPool(CommandPool* pool);
	void setLevel(VkCommandBufferLevel level);
	void begin();
	void end();
	void beginRenderPass(RenderPass* rp, VkFramebuffer fb);
	void endRenderPass();
	void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
	void setScissor(int32_t offsetx, int32_t offsety, uint32_t width, uint32_t height);
	void bindDescriptorSets(PipelineLayout* layout, std::vector<DescriptorSet*> sets, uint32_t firstSet = 0);
	void bindPipeline(Pipeline* pipeline);
	void bindVertexBuffer(Buffer& buffer, uint32_t binding, VkDeviceSize offset = 0);
	void bindIndexBuffer(Buffer& buffer, VkIndexType indexType, VkDeviceSize offset = 0);
	void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
	void updatePushConstant(PipelineLayout* layout, uint32_t index, const void* values);
};