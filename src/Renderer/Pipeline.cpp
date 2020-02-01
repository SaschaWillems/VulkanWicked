/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Pipeline.h"

Pipeline::Pipeline(VkDevice device) {
	this->device = device;
}

Pipeline::~Pipeline() {
	// @todo: destroy shader modules
	vkDestroyPipeline(device, pso, nullptr);
}

void Pipeline::create() {
	assert(layout);
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.layout = layout->handle;
	pipelineCI.renderPass = renderPass->handle;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, cache, 1, &pipelineCI, nullptr, &pso));
}

void Pipeline::addShader(std::string filename) {
	size_t extpos = filename.find('.');
	size_t extend = filename.find('.', extpos + 1);
	assert(extpos != std::string::npos);
	std::string ext = filename.substr(extpos + 1, extend - extpos - 1);
	VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	if (ext == "vert") { shaderStage = VK_SHADER_STAGE_VERTEX_BIT; }
	if (ext == "frag") { shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT; }
	assert(shaderStage != VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);

	VkPipelineShaderStageCreateInfo shaderStageCI{};
	shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCI.stage = shaderStage;
	shaderStageCI.pName = "main";
	shaderStageCI.module = assetManager->loadShaderShaderModule(filename);
	assert(shaderStageCI.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStageCI.module);
	shaderStages.push_back(shaderStageCI);
}

void Pipeline::setLayout(PipelineLayout* layout) {
	this->layout = layout;
}

void Pipeline::setRenderPass(RenderPass* renderPass) {
	this->renderPass = renderPass;
}

void Pipeline::setCreateInfo(VkGraphicsPipelineCreateInfo pipelineCI) {
	this->pipelineCI = pipelineCI;
	this->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void Pipeline::setCache(VkPipelineCache cache) {
	this->cache = cache;
}

VkPipelineBindPoint Pipeline::getBindPoint() {
	return bindPoint;
}

VkPipeline Pipeline::getHandle() {
	return pso;
}
