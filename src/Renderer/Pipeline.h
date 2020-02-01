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
#include "AssetManager.h"
#include "PipelineLayout.h"
#include "RenderPass.h"

class Pipeline 
{
private:
	VkDevice device = VK_NULL_HANDLE;
	VkPipeline pso = VK_NULL_HANDLE;
	VkPipelineBindPoint bindPoint;
	PipelineLayout* layout = nullptr;
	RenderPass* renderPass;
	VkGraphicsPipelineCreateInfo pipelineCI;
	VkPipelineCache cache;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;
public:
	Pipeline(VkDevice device);
	~Pipeline();
	void create();
	void addShader(std::string filename);
	void setLayout(PipelineLayout* layout);
	void setRenderPass(RenderPass* renderPass);
	void setCreateInfo(VkGraphicsPipelineCreateInfo pipelineCI);
	void setCache(VkPipelineCache cache);
	VkPipelineBindPoint getBindPoint();
	VkPipeline getHandle();
};
