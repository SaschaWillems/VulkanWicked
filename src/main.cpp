/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <chrono>

#include <SDL.h>

#include "Renderer/AssetManager.h"
#include "Renderer/VulkanRenderer.h"

#include "Player.h"
#include "Guardian.h"
#include "PlayingField.h"
#include "TarotDeck.h"

#include "Game.h"
#include "GameState.h"
#include "GameInput.h"

#include "DebugUI.h"
#include "UI/GameUI.h"

Game* game;
Player* player;
Guardian* guardian;
TarotDeck* tarotDeck;
VulkanRenderer* renderer;

std::chrono::time_point tStart = std::chrono::high_resolution_clock::now();
std::chrono::duration<float, std::milli> tDelta;

void init()
{
	debugUI = new DebugUI();
	debugUI->setRenderer(renderer);

	gameUI = new UI::GameUI();
	gameUI->setRenderer(renderer);
	gameUI->addFont("Raleway-Bold");
	gameUI->setfont("Raleway-Bold");
	gameUI->addTextElement("player_score", "0500", glm::vec3(0.0f), UI::TextAlignment::TopLeft, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	gameUI->addTextElement("pause", "Paused", glm::vec3(0.5f, 0.5f, 0.0f), UI::TextAlignment::Center, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f), false);

	game = new Game();
	game->setRenderer(renderer);
	gameState = new GameState();
	// Visual bounding box
	// @todo: Different aspect ratios
	const float dim = 12.5f;
	const float ar = (float)renderer->width / (float)renderer->height;
	BoundingBox boundingBox(-dim * ar, dim * ar, -dim, dim);
	gameState->boundingBox = boundingBox;

	playingField = new PlayingField();
	playingField->setRenderer(renderer);
	playingField->generate(35, 19);

	player = new Player();
	player->setRenderer(renderer);

	guardian = new Guardian();
	guardian->setRenderer(renderer);

	tarotDeck = new TarotDeck();
	tarotDeck->setRenderer(renderer);

	input = new GameInput();
	input->addInputListener(game);
	input->addInputListener(player);

	game->player = player;
	game->guardian = guardian;

	std::srand((int)std::time(nullptr));

	renderer->camera.type = Camera::CameraType::firstperson;
	renderer->camera.position = { 0.f, 20.0f, 0.0f };
	renderer->camera.setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
	renderer->camera.setOrtho(boundingBox.left, boundingBox.right, boundingBox.top, boundingBox.bottom, -100.0f, 100.0f);

	gameState->windowSize = glm::vec2(renderer->width, renderer->height);

	debugUI->player = player;
	debugUI->guardian = guardian;
	debugUI->tarotDeck = tarotDeck;

	game->spawnPlayer();
	game->spawnGuardian();
}

void buildCommandBuffer()
{
	CommandBuffer* cb = renderer->commandBuffer;
	cb->begin();

	// Fill render-targets (offscreen)
	cb->beginRenderPass(renderer->getRenderPass("offscreen"), renderer->offscreenPass.frameBuffer);
	cb->setViewport(0.0f, 0.0f, (float)renderer->offscreenPass.width, (float)renderer->offscreenPass.height, 0.0f, 1.0f);
	cb->setScissor(0, 0, renderer->offscreenPass.width, renderer->offscreenPass.height);
	
	cb->bindPipeline(renderer->getPipeline("backdrop"));
	cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, player->descriptorSet }, 0);
	assetManager->getModel("plane")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);

	// Face
	// @todo: Separate pipeline
	cb->bindPipeline(renderer->getPipeline("backdrop"));
	switch (gameState->phase) {
	case Phase::Day:
	{
		assetManager->getModel("face_sun")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
		break;
	}
	case Phase::Night:
	{
		assetManager->getModel("face_moon")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
		break;
	}
	}

	// Playing field
	cb->bindVertexBuffer(playingField->instanceBuffer, 1);
	cb->bindPipeline(renderer->getPipeline("spore"));

	const std::vector<SporeType> sporetypes = { SporeType::Good, SporeType::Good_Portal, SporeType::Evil, SporeType::Evil_Portal, SporeType::Evil_Dead };
	for (auto sporetype : sporetypes) {
		vkglTF::Model* model = nullptr;
		// @todo: store model referene in cell upon change
		switch (sporetype) {
		case SporeType::Good:
			model = assetManager->getModel("spore_good");
			break;
		case SporeType::Good_Portal:
			model = assetManager->getModel("portal_good");
			break;
		case SporeType::Evil:
			model = assetManager->getModel("spore_evil");
			break;
		case SporeType::Evil_Dead:
			model = assetManager->getModel("spore_evil_dead");
			break;
		case SporeType::Evil_Portal:
			model = assetManager->getModel("portal_evil");
			break;
		}
		model->bindBuffers(cb->handle);
		for (uint32_t x = 0; x < playingField->width; x++) {
			for (uint32_t y = 0; y < playingField->height; y++) {
				Cell* cell = &playingField->cells[x][y];
				if (cell->sporeType == sporetype) {
					const uint32_t idx = (x * playingField->height) + y;
					model->drawNodes(cb->handle, renderer->getPipelineLayout("split_ubo")->handle, idx);
				}
			}
		}
	}

	//@todo: Virtual function in RenderObject class, register, Renderobjects and draw in loop
	tarotDeck->draw(cb);
	player->draw(cb);
	guardian->draw(cb);

	// Projectiles
	if (!gameState->projectiles.empty()) {
		//sassetManager->getModel("projectile_player")->bindBuffers(cb->handle);
		cb->bindPipeline(renderer->getPipeline("projectile"));
		cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, game->descriptorSetProjectiles }, 0);
		for (uint32_t i = 0; i < gameState->projectiles.size(); i++) {
			Projectile& projectile = gameState->projectiles[i];
			vkglTF::Model* model = assetManager->getModel("projectile_player");
			// @todo
			if (projectile.type == ProjectileType::Good_Portal_Spawn) {
				model = assetManager->getModel("portal_spawner_good");
			}
			model->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle, i);
			//assetManager->getModel("projectile_player")->drawNodes(cb->handle, renderer->getPipelineLayout("split_ubo")->handle, i);
		}
	}

	cb->endRenderPass();

	// Deferred composition
	cb->beginRenderPass(renderer->getRenderPass("deferred_composition"), renderer->frameBuffers[renderer->currentBuffer]);
	cb->setViewport(0.0f, 0.0f, (float)renderer->width, (float)renderer->height, 0.0f, 1.0f);
	cb->setScissor(0, 0, renderer->width, renderer->height);
	cb->bindDescriptorSets(renderer->getPipelineLayout("deferred_composition"), { renderer->deferredComposition.descriptorSet }, 0);
	cb->bindPipeline(renderer->getPipeline(game->paused ? "composition_paused" : "composition"));
	cb->draw(6, 1, 0, 0);
	if (renderer->settings.debugoverlay) {
		debugUI->draw(cb);
	}
	gameUI->draw(cb);
	cb->endRenderPass();

	cb->end();
}

void updateLights()
{
	renderer->lightSources.numLights = 0;
	renderer->lightSources.viewPos = glm::vec4(renderer->camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	renderer->addLight(player->getLightSource());
	renderer->addLight(guardian->getLightSource());
	renderer->addLight(game->getPhaseLight());
	for (auto projectile : gameState->projectiles) {
		if (projectile.alive) {
			renderer->addLight(projectile.getLightSource());
		}
	}
	for (uint32_t x = 0; x < playingField->width; x++) {
		for (uint32_t y = 0; y < playingField->height; y++) {
			Cell& cell = playingField->cells[x][y];
			if (cell.hasLightSource()) {
				renderer->addLight(cell.getLightSource());
			}
		}
	}
	renderer->deferredComposition.lightsBuffer.copyTo(&renderer->lightSources, sizeof(renderer->lightSources));
}

int SDL_main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return -1;
	}

	for (int32_t i = 0; i < __argc; i++) {
		VulkanRenderer::args.push_back(__argv[i]);
	};

	assetManager = new AssetManager();
	renderer = new VulkanRenderer();

	init();

	assetManager->addModelsFolder("scenes");
	assetManager->addTexturesFolder("textures");

	guardian->setModel("guardian_black_sun");

	game->prepareGPUResources();
	debugUI->prepareGPUResources(renderer->pipelineCache, renderer->getRenderPass("deferred_composition"));
	playingField->prepareGPUResources();
	player->prepareGPUResources();
	guardian->prepareGPUResources();
	tarotDeck->prepareGPUResources();
	gameUI->prepareGPUResources();

	renderer->camera.updateGPUResources();
	player->updateGPUResources();
	guardian->updateGPUResources();
	tarotDeck->updateGPUResources();
	gameUI->updateGPUResources();

	bool minimized = false;
	bool quit = false;
	while (!quit) {
		tDelta = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - tStart);
		tStart = std::chrono::high_resolution_clock::now();
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent)) {
			switch (sdlEvent.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT: {
				if (sdlEvent.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					minimized = true;
				}
				if (sdlEvent.window.event == SDL_WINDOWEVENT_RESTORED) {
					minimized = false;
				}
				break;
			}
			default:
				input->handleInput(sdlEvent);
			}
		}

		if (minimized) {
			continue;
		}

		renderer->waitSync();

		debugUI->render();
		buildCommandBuffer();

		input->update();

		if (!game->paused) {
			game->updateGPUResources();
			updateLights();
		}

		renderer->submitFrame();

		float timeStep = tDelta.count() / 1000.0f;
		if (!game->paused) {
			playingField->update(timeStep);
			game->update(timeStep);
			player->update(timeStep);
			guardian->update(timeStep);
			tarotDeck->update(timeStep);
		}
		gameUI->updateGPUResources();
	}

	delete playingField;
	delete game;
	delete player;
	delete guardian;
	delete debugUI;
	delete gameUI;
	delete renderer;
	delete input;

	return 0;
}