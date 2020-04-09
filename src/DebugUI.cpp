/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "DebugUI.h"

DebugUI* debugUI = nullptr;

DebugUI::DebugUI()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.0f;
	timing.fps.format = "%.1f";
}

DebugUI::~DebugUI()
{ 
	ImGui::DestroyContext();
	vertexBuffer.destroy();
	indexBuffer.destroy();
	delete sampler;
	delete fontImage;
	delete fontImageView;
}

void DebugUI::prepareGPUResources(const VkPipelineCache pipelineCache, RenderPass* renderPass)
{
	ImGuiIO& io = ImGui::GetIO();

	// Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
#if defined(__ANDROID__)
	float scale = (float)vks::android::screenDensity / (float)ACONFIGURATION_DENSITY_MEDIUM;
	AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, "Roboto-Medium.ttf", AASSET_MODE_STREAMING);
	if (asset) {
		size_t size = AAsset_getLength(asset);
		assert(size > 0);
		char* fontAsset = new char[size];
		AAsset_read(asset, fontAsset, size);
		AAsset_close(asset);
		io.Fonts->AddFontFromMemoryTTF(fontAsset, size, 14.0f * scale);
		delete[] fontAsset;
	}
#else
	io.Fonts->AddFontFromFileTTF(std::string(assetManager->assetPath + "Roboto-Medium.ttf").c_str(), 16.0f);
#endif		
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

	fontImage = new Image(renderer->device);
	fontImage->setType(VK_IMAGE_TYPE_2D);
	fontImage->setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	fontImage->setExtent({ (uint32_t)texWidth, (uint32_t)texHeight, 1 });
	fontImage->setTiling(VK_IMAGE_TILING_OPTIMAL);
	fontImage->setUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	fontImage->create();

	fontImageView = new ImageView(renderer->device);
	fontImageView->setType(VK_IMAGE_VIEW_TYPE_2D);
	fontImageView->setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	fontImageView->setSubResourceRange({ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	fontImageView->setImage(fontImage);
	fontImageView->create();

	// Staging buffers for font data upload
	Buffer stagingBuffer;
	VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingBuffer, uploadSize));
	memcpy(stagingBuffer.mapped, fontData, uploadSize);

	// Copy buffer data to font image
	VkCommandBuffer copyCmd = renderer->device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Prepare for transfer
	vks::tools::setImageLayout(
		copyCmd,
		fontImage->handle,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	// Copy
	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = texWidth;
	bufferCopyRegion.imageExtent.height = texHeight;
	bufferCopyRegion.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer.buffer,
		fontImage->handle,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);

	// Prepare for shader read
	vks::tools::setImageLayout(
		copyCmd,
		fontImage->handle,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	renderer->device->flushCommandBuffer(copyCmd, renderer->queue, true);

	stagingBuffer.destroy();

	sampler = new Sampler(renderer->device);
	sampler->setMinFilter(VK_FILTER_LINEAR);
	sampler->setMagFilter(VK_FILTER_LINEAR);
	sampler->setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	sampler->setBorderColor(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
	sampler->create();

	descriptorPool = new DescriptorPool(renderer->device->handle);
	descriptorPool->setMaxSets(1);
	descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
	descriptorPool->create();

	descriptorSetLayout = new DescriptorSetLayout(renderer->device->handle);
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	VkDescriptorImageInfo fontDescriptor = vks::initializers::descriptorImageInfo(sampler->handle, fontImageView->handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	descriptorSet = new DescriptorSet(renderer->device->handle);
	descriptorSet->setPool(descriptorPool);
	descriptorSet->addLayout(descriptorSetLayout);
	descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &fontDescriptor);
	descriptorSet->create();

	pipelineLayout = new PipelineLayout(renderer->device->handle);
	pipelineLayout->addLayout(descriptorSetLayout);
	pipelineLayout->addPushConstantRange(sizeof(PushConstBlock), 0, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineLayout->create();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState =
		vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colorBlendState =
		vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState =
		vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
	VkPipelineViewportStateCreateInfo viewportState =
		vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState =
		vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState =
		vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;

	std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
		vks::initializers::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	// Location 0: Position
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	// Location 1: UV
		vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),	// Location 0: Color
	};
	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();
	pipelineCI.pVertexInputState = &vertexInputState;

	pipeline = new Pipeline(renderer->device->handle);
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(pipelineLayout);
	pipeline->setRenderPass(renderPass);
	pipeline->addShader("shaders/base/uioverlay.vert.spv");
	pipeline->addShader("shaders/base/uioverlay.frag.spv");
	pipeline->create();
}

void DebugUI::updateGPUResources()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	if (!imDrawData) { return; };

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	// Update buffers only if vertex or index count has been changed compared to current buffer size
	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}

	// Vertex buffer
	if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
		vertexBuffer.destroy();
		VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &vertexBuffer, vertexBufferSize));
		vertexCount = imDrawData->TotalVtxCount;
	}

	// Index buffer
	VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
		indexBuffer.destroy();
		VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &indexBuffer, indexBufferSize));
		indexCount = imDrawData->TotalIdxCount;
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertexBuffer.flush();
	indexBuffer.flush();
}

void DebugUI::onMouseButtonClick(uint32_t button)
{
	if (button == SDL_BUTTON_MIDDLE) {
		glm::vec2 pos = (glm::vec2(mousePos) / gameState->windowSize) - glm::vec2(0.5f);
		pos.x = pos.x * (float)playingField->width;
		pos.y = pos.y * (float)playingField->height;
		glm::ivec2 cpos = glm::vec2(round(pos.x) + floor(playingField->width / 2), round(pos.y) + floor(playingField->height / 2));
		selectedCell = playingField->cellAt(cpos);
	}
}

void DebugUI::draw(CommandBuffer* cb)
{
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);

	cb->setViewport(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 1.0f);
	cb->setScissor(0, 0, (int32_t)io.DisplaySize.x, int32_t(io.DisplaySize.y));
	cb->bindPipeline(pipeline);
	cb->bindDescriptorSets(pipelineLayout, { descriptorSet });
	cb->updatePushConstant(pipelineLayout, 0, &pushConstBlock);
	cb->bindVertexBuffer(vertexBuffer, 0);
	cb->bindIndexBuffer(indexBuffer, VK_INDEX_TYPE_UINT16);

	for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
			cb->setScissor(std::max((int32_t)(pcmd->ClipRect.x), 0), std::max((int32_t)(pcmd->ClipRect.y), 0), (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x), (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y));
			cb->drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
			indexOffset += pcmd->ElemCount;
		}
		vertexOffset += cmd_list->VtxBuffer.Size;
	}
}

bool ColoredButton(const char* label, ImVec4 color)
{
	bool res;
	ImGui::PushStyleColor(ImGuiCol_Button, color);
	res = ImGui::Button(label, ImVec2(100.0f, 25.0f));
	ImGui::PopStyleColor();
	return res;
}

void DisplayPerformanceValue(const std::string &caption, PerformanceValue &value) 
{
	ImGui::Text(std::string(caption + ": " + value.format + " (" + value.format + "/" + value.format + ")").c_str(), value.current, value.min, value.max);
}

const char* cellSporeTypeAsString(SporeType sporeType) {
	switch (sporeType) {
	case SporeType::Empty:
		return "Empty";
	case SporeType::Good:
		return "Good";
	case SporeType::Good_Portal:
		return "Good Portal";
	case SporeType::Evil:
		return "Evil";
	case SporeType::Evil_Portal:
		return "Evil Portal";
	case SporeType::Evil_Dead:
		return "Evil Dead";
	case SporeType::Deadzone:
		return "Deadzone";
	default:
		return "";
	}
}

void cellInfo(Cell* cell, bool showOwner, bool showNeighbours)
{
	if (!cell) {
		return;
	}
	ImGui::Text("Pos: %d / %d", cell->pos.x, cell->pos.y);
	ImGui::Text("GridPos.: %f / %f", cell->gridPos.x, cell->gridPos.y);
	ImGui::Text("ZIndex: %f", cell->zIndex);
	ImGui::Text("Type: %s", cellSporeTypeAsString(cell->sporeType));
	if (showOwner) {
		if (cell && cell->owner) {
			if (ImGui::CollapsingHeader("Owner", ImGuiTreeNodeFlags_DefaultOpen)) {
				cellInfo(cell->owner, false, false);
			}
		}
	}
	if (showNeighbours) {
		if (ImGui::CollapsingHeader("Neighbours", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (auto neighbour : cell->neighbours) {
				if (neighbour) {
					ImGui::Text("%+d / %+d - %s", neighbour->pos.x - cell->pos.x, neighbour->pos.y - cell->pos.y, cellSporeTypeAsString(neighbour->sporeType));
				}
			}
		}
	}
}

void DebugUI::render()
{
	ImVec2 btnSize = ImVec2(100.0f, 25.0f);
	ImVec4 colorGood = ImVec4(1.0f, 0.6f, 0.24f, 1.0f);
	ImVec4 colorEvil = ImVec4(0.0f, 0.0f, 0.74f, 1.0f);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(gameState->windowSize.x, gameState->windowSize.y);
	io.MousePos = ImVec2(input->mouse.position.x, input->mouse.position.y);
	io.MouseDown[0] = input->mouse.buttons.left;
	io.MouseDown[1] = input->mouse.buttons.right;

	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_None);
	DisplayPerformanceValue("fps", timing.fps);
	if (ImGui::CollapsingHeader("Deatils", ImGuiTreeNodeFlags_DefaultOpen)) {
		DisplayPerformanceValue("cb build", timing.commandbufferbuild);
		DisplayPerformanceValue("playfield update", timing.playfieldupdate);
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("General info", nullptr, ImGuiWindowFlags_None);
	ImGui::Text("phase: %s", gameState->phase == Phase::Day ? "day" : "night");
	ImGui::Text("phaseT: %f", gameState->phaseTimer);
	uint32_t pcount = 0;
	for (auto p : gameState->projectiles) {
		if (p.alive) {
			pcount++;
		}
	}
	ImGui::Text("projectiles (alive): %d (%d)", gameState->projectiles.size(), pcount);
	ImGui::End();

	if (selectedCell) {
		ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Cell", nullptr, ImGuiWindowFlags_None);
		cellInfo(selectedCell, true, true);
		ImGui::End();
	}

	ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None);
	ImGui::Text("Pos: %f / %f / %f", player->position.x, player->position.y, player->position.z);
	ImGui::Text("Health: %f", player->health);
	ImGui::Text("State: %d", (int32_t)player->state);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Guardian", nullptr, ImGuiWindowFlags_None);
	ImGui::Text("Pos: %f / %f / %f", guardian->position.x, guardian->position.y, guardian->position.z);
	ImGui::Text("Health: %f", guardian->health);
	ImGui::Text("State: %d", (int32_t)guardian->state);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(45, 45), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Tarot deck", nullptr, ImGuiWindowFlags_None);
	ImGui::Text("State: %d (timer = %f)", (int32_t)tarotDeck->state, tarotDeck->stateTimer);
	if (ImGui::Button("Appear", btnSize)) {
		tarotDeck->setState(TarotDeckState::Appearing);
	}
	if (ImGui::Button("Disappear", btnSize)) {
		tarotDeck->setState(TarotDeckState::Disappearing);
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(55, 55), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Functions", nullptr, ImGuiWindowFlags_None);

	if (ImGui::CollapsingHeader("Gamestate", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ColoredButton("Phase day", colorGood)) {
			gameState->phase = Phase::Day;
			gameState->phaseTimer = gameState->values.phaseDuration;
		}
		ImGui::SameLine();
		if (ColoredButton("Phase night", colorEvil)) {
			gameState->phase = Phase::Night;
			gameState->phaseTimer = gameState->values.phaseDuration;
		}
	}

	if (ImGui::CollapsingHeader("Spawn", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ColoredButton("Good portal", colorGood)) {
			Cell* cell = playingField->cellFromVisualPos(player->position);
			if (cell) {
				cell->sporeType = SporeType::Good_Portal;
				cell->sporeSize = gameState->values.maxSporeSize;
			}
		}
		ImGui::SameLine();
		if (ColoredButton("Evil portal", colorEvil)) {
			Cell* cell = playingField->cellFromVisualPos(player->position);
			if (cell) {
				cell->sporeType = SporeType::Evil_Portal;
				cell->sporeSize = gameState->values.maxSporeSize;
			}
		}
		if (ColoredButton("Good spore", colorGood)) {
			Cell* cell = playingField->cellFromVisualPos(player->position);
			if (cell) {
				cell->sporeType = SporeType::Good;
				cell->sporeSize = gameState->values.maxSporeSize;
			}
		}
		ImGui::SameLine();
		if (ColoredButton("Evil spore", colorEvil)) {
			Cell* cell = playingField->cellFromVisualPos(player->position);
			if (cell) {
				cell->sporeType = SporeType::Evil;
				cell->sporeSize = gameState->values.maxSporeSize;
			}
		}
	}

	if (ImGui::CollapsingHeader("Playing field", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Reset", btnSize)) {
			for (uint32_t x = 0; x < playingField->width; x++) {
				for (uint32_t y = 0; y < playingField->height; y++) {
					Cell& cell = playingField->cells[x][y];
					cell.sporeSize = 0.0f;
					cell.sporeType = SporeType::Empty;
					cell.floatValue = 0.0f;
				}
			}
		}
		if (ImGui::Button("Save", btnSize)) {
			std::ofstream file;
			file = std::ofstream("playingfield.dat", std::ios::out | std::ios::binary);
			playingField->save(file);
			file.close();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load", btnSize)) {
			std::ifstream file;
			file.open("playingfield.dat", std::ios::in | std::ios::binary);
			if (file.is_open()) {
				playingField->load(file);
			}
			file.close();
		}
	}

	if (ImGui::CollapsingHeader("Level", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginCombo("##levelfile", selectedLevelName.c_str())) {
			for (auto level : game->levels) {
				bool isSelected = (selectedLevelName == level.second);
				if (ImGui::Selectable(level.first.c_str(), isSelected)) {
					selectedLevelName = level.first;
					selectedLevelFile = level.second;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load##level", btnSize)) {
			game->loadLevel(selectedLevelFile);
		}
	}


	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(70, 70), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Game state settings", nullptr, ImGuiWindowFlags_None);
	ImGui::PushItemWidth(110.0f);
	ImGui::InputFloat("maxSporeSize", &gameState->values.maxSporeSize, 0.1f, 1.0f, 2);
	ImGui::InputFloat("maxGrowthDistanceToPortal", &gameState->values.maxGrowthDistanceToPortal, 0.1f, 1.0f, 2);
	ImGui::InputFloat("maxNumGoodPortalSpawners", &gameState->values.maxNumGoodPortalSpawners, 0.1f, 1.0f, 2);
	ImGui::InputFloat("maxNumEvilPortalSpawners", &gameState->values.maxNumEvilPortalSpawners, 0.1f, 1.0f, 2);
	ImGui::InputFloat("growthSpeedFast", &gameState->values.growthSpeedFast, 0.1f, 1.0f, 2);
	ImGui::InputFloat("growthSpeedSlow", &gameState->values.growthSpeedSlow, 0.1f, 1.0f, 2);
	ImGui::InputFloat("phaseDuration", &gameState->values.phaseDuration, 0.1f, 1.0f, 2);
	ImGui::InputFloat("spawnTimer", &gameState->values.spawnTimer, 0.1f, 1.0f, 2);
	ImGui::InputFloat("evilPortalSpawnerSpawnChance", &gameState->values.evilPortalSpawnerSpawnChance, 0.1f, 1.0f, 2);
	ImGui::InputFloat("evilPortalSpawnerSpeed", &gameState->values.evilPortalSpawnerSpeed, 0.1f, 1.0f, 2);
	ImGui::InputFloat("evilDeadSporeLife", &gameState->values.evilDeadSporeLife, 0.1f, 1.0f, 2);
	ImGui::InputFloat("evilDeadSporeRessurectionSpeed", &gameState->values.evilDeadSporeRessurectionSpeed, 0.1f, 1.0f, 2);
	ImGui::InputFloat("playerFiringCooldown", &gameState->values.playerFiringCooldown, 0.1f, 1.0f, 2);
	ImGui::InputFloat("playerProjectileSpeed", &gameState->values.playerProjectileSpeed, 0.1f, 1.0f, 2);
	ImGui::PopItemWidth();
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Renderer", nullptr, ImGuiWindowFlags_None);
	if (ImGui::Button("Export VMA stats", btnSize)) {
		char* statsStr;
		std::ofstream ofstream;
		ofstream.open("vmastats.json");
		if (ofstream.is_open()) {
			vmaBuildStatsString(renderer->device->vmaAllocator, &statsStr, VK_TRUE);
			ofstream << statsStr;
			vmaFreeStatsString(renderer->device->vmaAllocator, statsStr);
			ofstream.close();
		}
	}
	ImGui::End();

	ImGui::EndFrame();
	ImGui::Render();

	updateGPUResources();
}
