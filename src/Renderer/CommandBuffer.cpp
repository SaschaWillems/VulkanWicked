/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "CommandBuffer.h"

CommandBuffer::CommandBuffer(VkDevice device) {
	this->device = device;
}

CommandBuffer::~CommandBuffer() {
	vkFreeCommandBuffers(device, pool->handle, 1, &handle);
}

void CommandBuffer::create() {
	assert(pool);
	VkCommandBufferAllocateInfo AI = vks::initializers::commandBufferAllocateInfo(pool->handle, level, 1);
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &AI, &handle));
}

void CommandBuffer::setPool(CommandPool* pool) {
	this->pool = pool;
}

void CommandBuffer::setLevel(VkCommandBufferLevel level) {
	this->level = level;
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo beginInfo = vks::initializers::commandBufferBeginInfo();
	VK_CHECK_RESULT(vkBeginCommandBuffer(handle, &beginInfo));
}

void CommandBuffer::end() {
	VK_CHECK_RESULT(vkEndCommandBuffer(handle));
}

void CommandBuffer::beginRenderPass(RenderPass* rp, VkFramebuffer fb) {
	rp->setFrameBuffer(fb);
	VkRenderPassBeginInfo beginInfo = rp->getBeginInfo();
	vkCmdBeginRenderPass(handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::endRenderPass() {
	vkCmdEndRenderPass(handle);
}

void CommandBuffer::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
	VkViewport viewport = { x, y, width, height, minDepth, maxDepth };
	vkCmdSetViewport(handle, 0, 1, &viewport);
}

void CommandBuffer::setScissor(int32_t offsetx, int32_t offsety, uint32_t width, uint32_t height) {
	VkRect2D scissor = { offsetx, offsety, width, height };
	vkCmdSetScissor(handle, 0, 1, &scissor);
}

void CommandBuffer::bindDescriptorSets(PipelineLayout* layout, std::vector<DescriptorSet*> sets, uint32_t firstSet) {
	std::vector<VkDescriptorSet> descSets;
	for (auto set : sets) {
		descSets.push_back(set->handle);
	}
	vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->handle, firstSet, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
}

void CommandBuffer::bindPipeline(Pipeline* pipeline) {
	vkCmdBindPipeline(handle, pipeline->getBindPoint(), pipeline->getHandle());
}

void CommandBuffer::bindVertexBuffer(Buffer& buffer, uint32_t binding, VkDeviceSize offset)
{
	vkCmdBindVertexBuffers(handle, binding, 1, &buffer.buffer, &offset);
}

void CommandBuffer::bindIndexBuffer(Buffer& buffer, VkIndexType indexType, VkDeviceSize offset)
{
	vkCmdBindIndexBuffer(handle, buffer.buffer, offset, indexType);
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	vkCmdDraw(handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::updatePushConstant(PipelineLayout* layout, uint32_t index, const void* values) {
	VkPushConstantRange pushConstantRange = layout->getPushConstantRange(index);
	vkCmdPushConstants(handle, layout->handle, pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.size, values);
}