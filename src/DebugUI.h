/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "imgui.h"
#include "GameState.h"
#include "GameInput.h"
#include "Player.h"
#include "Guardian.h"
#include "TarotDeck.h"
#include "PlayingField.h"
#include "Renderer/VulkanTools.h"
#include "Renderer/AssetManager.h"
#include "Renderer/PipelineLayout.h"
#include "Renderer/Pipeline.h"
#include "Renderer/RenderPass.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/DescriptorSetLayout.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/DescriptorPool.h"
#include "Renderer/RenderObject.h"
#include "Renderer/Image.h"
#include "Renderer/ImageView.h"
#include "Renderer/Sampler.h"

class DebugUI: public RenderObject
{
private:
	bool visible = true;
	Buffer vertexBuffer;
	Buffer indexBuffer;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	DescriptorPool* descriptorPool;
	DescriptorSetLayout* descriptorSetLayout;
	DescriptorSet* descriptorSet;
	PipelineLayout* pipelineLayout;
	Pipeline* pipeline;
	Image* fontImage;
	ImageView* fontImageView;
	Sampler* sampler;
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;
	void updateGPUResources();
public:
	Player* player;
	Guardian* guardian;
	TarotDeck* tarotDeck;
	DebugUI();
	~DebugUI();
	void render();
	void prepareGPUResources(const VkPipelineCache pipelineCache, RenderPass* renderPass);
	void draw(CommandBuffer* cb);
};

extern DebugUI* debugUI;

