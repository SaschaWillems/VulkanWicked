/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Game.h"

LightSource Game::getPhaseLight()
{
	// @todo: Fade colors at phase shift
	LightSource lightSource;
	lightSource.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	lightSource.color = gameState->phase == Phase::Day ? glm::vec3(1.0f, 0.7f, 0.3f) : glm::vec3(0.0f, 0.0f, 1.0f);
	lightSource.radius = 30.0f;
	return lightSource;
}

void Game::spawnTrigger()
{
	if (gameState->phase == Phase::Day) {
		std::cout << "Spawn Trigger day" << std::endl;
		for (uint32_t x = 0; x < playingField->width; x++) {
			for (uint32_t y = 0; y < playingField->height; y++) {
				Cell& cell = playingField->cells[x][y];
				// Randomly spawn good portal spawn projectiles at one good portal
				if ((cell.sporeType == SporeType::Good_Portal) && (gameState->projectileCountByType(ProjectileType::Good_Portal_Spawn) < gameState->values.maxNumGoodPortalSpawners)) {
					if (randomFloat(1.0f) < gameState->values.evilPortalSpawnerSpawnChance) {
						std::clog << "Spawned good portal spawner" << std::endl;
						glm::vec3 pos = { cell.gridPos.x, cell.zIndex + 0.1f, cell.gridPos.y };
						glm::vec3 dir = glm::vec3(0.0f);
						// @todo: Only spawn if portal has a min. age?
						gameState->addProjectile(Projectile(pos, dir, ProjectileType::Good_Portal_Spawn));
					}
				}
			}
		}
	}
	if (gameState->phase == Phase::Night) {
		std::cout << "Spawn Trigger night" << std::endl;
		for (uint32_t x = 0; x < playingField->width; x++) {
			for (uint32_t y = 0; y < playingField->height; y++) {
				Cell& cell = playingField->cells[x][y];
				// Randomly spawn evil portal spawn projectiles for every evil portal
				if ((cell.sporeType == SporeType::Evil_Portal) && (gameState->projectileCountByType(ProjectileType::Evil_Portal_Spawn) < gameState->values.maxNumEvilPortalSpawners)) {
					if (randomFloat(1.0f) < gameState->values.evilPortalSpawnerSpawnChance) {
						std::clog << "Spawned evil portal spawner" << std::endl;
						glm::vec3 pos = { cell.gridPos.x, -128.0f, cell.gridPos.y };
						glm::vec3 dir = glm::vec3(0.0f);
						dir.x = randomFloat(1.0f) < 0.5f ? -1.0f : 1.0f;
						dir.z = randomFloat(1.0f) < 0.5f ? -1.0f : 1.0f;
						// @todo: Only spawn if portal has a min. age?
						gameState->addProjectile(Projectile(pos, dir, ProjectileType::Evil_Portal_Spawn));
					}
				}
			}
		}
	}
}

void Game::update(float dT)
{
	// Fade between views
	const float fadeSpeed = dT * 0.5f;
	if (view != targetView) {
		fade -= fadeSpeed;
		if (fade <= 0.0f) {
			view = targetView;
			fade = 0.0f;
		}
	}
	else {
		if (fade < 1.0f) {
			fade += fadeSpeed;
			if (fade > 1.0f) {
				fade = 1.0f;
			}
		}
	}
	if (view == View::InGame) {
		updateProjectiles(dT);
		updateSpawnTimer(dT);
		updateState(dT);
		updateTarotDeck(dT);
	}
}

void Game::updateSpawnTimer(float dT)
{
	gameState->spawnTimer -= dT;
	if (gameState->spawnTimer < 0.0f) {
		spawnTrigger();
		gameState->spawnTimer = gameState->values.spawnTimer;
	}
}

void Game::updateState(float dT)
{
	gameState->phaseTimer -= 1.5f * dT;
	if (gameState->phaseTimer < 0.0f) {
		gameState->phaseTimer = gameState->values.phaseDuration;
		if (gameState->phase == Phase::Day) {
			gameState->phase = Phase::Night;
		}
		else {
			gameState->phase = Phase::Day;
		}
	}
}

void Game::onKeyPress(SDL_Keycode keyCode)
{
	switch(keyCode) 
	{
	case SDLK_p:
		paused = !paused;
		//@todo
		gameUI->getTextElement("pause")->visible = paused;
		break;
	case SDLK_F1:
		renderer->settings.debugoverlay = !renderer->settings.debugoverlay;
		break;
	}
}

void Game::updateProjectiles(float dT)
{
	if (gameState->projectiles.empty()) {
		return;
	}
	for (auto& projectile : gameState->projectiles) {
		if (projectile.alive) {
			switch (projectile.type) {
			case ProjectileType::Player: {
				// Player projectiles move in set direction
				// @todo: Collision with e.g. evil spores
				// @todo: acceleration
				projectile.pos += projectile.dir * gameState->values.playerProjectileSpeed * dT;
				if (!gameState->boundingBox.inside(projectile.pos, 0.25f)) {
					projectile.remove();
					continue;
				}
				// Player projectiles destroy evil portal spawners
				// @todo: scoring?
				for (auto& otherProjectile : gameState->projectiles) {
					if (otherProjectile.type == ProjectileType::Evil_Portal_Spawn && otherProjectile.alive) {
						if (projectile.distanceTo(otherProjectile) < gameState->values.evilSpawnerProjectileSize) {
							projectile.remove();
							otherProjectile.remove();
							continue;
						}
					}
				}
				// Player projectiles damage guardian
				// @todo: Guardian hit test precedence over spore hit test
				if (guardian->alive() && guardian->hitTest(projectile.pos)) {
					guardian->health -= gameState->values.playerProjectileGuardianDamage;
					projectile.remove();
					continue;
				}
				// Player projectiles turn evil spores temporarily into dead evil spores that can be taken over by good growth
				Cell* cell = playingField->cellFromVisualPos(projectile.pos);
				if (cell && cell->sporeType == SporeType::Evil) {
					cell->sporeType = SporeType::Evil_Dead;
					cell->sporeSize = 1.0f;
					cell->floatValue = gameState->values.evilDeadSporeLife;
					projectile.remove();
					continue;
				}
				}
				break;
			case ProjectileType::Evil_Portal_Spawn:
				// @todo: Portal can only be spawned if projectile has at least bounced once (e.g. general "value" prop on projectile)?
				// @todo: Function to map visual pos to cell pos
				// @todo: Spore hit detection
				// When bounced at least once touchung an evil spore turns it into a new portal
				if (projectile.intValue >= 1) {
					Cell* cell = playingField->cellFromVisualPos(projectile.pos);
					if (cell && cell->sporeType == SporeType::Evil) {
						// @todo: Function to encapsulate spore type change?
						cell->sporeType = SporeType::Evil_Portal;
						projectile.remove();
						continue;
					}
				}
				// @todo: Maybe make this a parameter
				// Evil portal spawn projectiles bounce off at borders until they hit an evil spore to turn it into a portal
				projectile.pos += projectile.dir * gameState->values.evilPortalSpawnerSpeed * dT;
				if ((projectile.pos.x <= gameState->boundingBox.left && projectile.dir.x < 0.0f) || (projectile.pos.x >= gameState->boundingBox.right && projectile.dir.x > 0.0f)) {
					projectile.dir.x = -projectile.dir.x;
					projectile.intValue++;
				}
				if ((projectile.pos.z <= gameState->boundingBox.top && projectile.dir.z < 0.0f) || (projectile.pos.z >= gameState->boundingBox.bottom && projectile.dir.z > 0.0f)) {
					projectile.dir.z = -projectile.dir.z;
					projectile.intValue++;
				}
				break;
			}
		}
	}
}

void Game::prepareGPUResources()
{
	VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &projectilesUbo, gameState->values.maxNumProjectiles * sizeof(glm::vec4)));
	descriptorSetProjectiles = new DescriptorSet(renderer->device->handle);
	descriptorSetProjectiles->setPool(renderer->descriptorPool);
	descriptorSetProjectiles->addLayout(renderer->getDescriptorSetLayout("single_ubo"));
	descriptorSetProjectiles->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &projectilesUbo.descriptor);
	descriptorSetProjectiles->create();
}

void Game::updateGPUResources()
{
	playingField->updateGPUResources();
	if (gameState->projectiles.size() > 0) {
		std::vector<glm::vec4> uniformdata(gameState->projectiles.size());
		for (size_t i = 0; i < gameState->projectiles.size(); i++) {
			uniformdata[i] = glm::vec4(gameState->projectiles[i].pos, 0.0f);
		}
		projectilesUbo.copyTo(uniformdata.data(), uniformdata.size() * sizeof(glm::vec4));
	}
}

void Game::spawnPlayer()
{
	glm::vec2 spawnPosition = glm::vec2(0.0f);
	// Player spawns at random good portal
	std::vector<Cell> spawnPoints;
	for (uint32_t x = 0; x < playingField->width; x++) {
		for (uint32_t y = 0; y < playingField->height; y++) {
			Cell& cell = playingField->cells[x][y];
			if (cell.sporeType == SporeType::Good_Portal) {
				spawnPoints.push_back(cell);
			}
		}
	}
	if (!spawnPoints.empty()) {
		int32_t index = randomInt(spawnPoints.size());
		spawnPosition = spawnPoints[index].gridPos;
		std::clog << "Spawning player at " << spawnPosition.x << " / " << spawnPosition.y << std::endl;
	}
	else {
		std::cerr << "No spawn point found for player, spawning at center" << std::endl;
	}
	player->spawn(spawnPosition);
}

void Game::spawnGuardian()
{
	glm::vec2 spawnPosition = glm::vec2(0.0f);
	// Guardian spawns at random evil portal
	std::vector<Cell> spawnPoints;
	for (uint32_t x = 0; x < playingField->width; x++) {
		for (uint32_t y = 0; y < playingField->height; y++) {
			Cell& cell = playingField->cells[x][y];
			if (cell.sporeType == SporeType::Evil_Portal) {
				spawnPoints.push_back(cell);
			}
		}
	}
	if (!spawnPoints.empty()) {
		int32_t index = randomInt(spawnPoints.size());
		spawnPosition = spawnPoints[index].gridPos;
		std::clog << "Spawning guardian at " << spawnPosition.x << " / " << spawnPosition.y << std::endl;
	}
	else {
		std::cerr << "No spawn point found for guardian, spawning at center" << std::endl;
	}
	guardian->spawn(spawnPosition);
}

void Game::setView(View view, bool fade)
{
	if (fade) {
		fade = 1.0f;
		this->targetView = view;
	}
	else {
		this->targetView = view;
		this->view = view;
	}
}

void Game::loadLevel(const std::string& filename)
{
	std::clog << "Loading level from \"" << filename << "\"" << std::endl;
	std::ifstream is(assetManager->assetPath + "/levels/" + filename);
	if (!is.is_open())
	{
		std::cerr << "Error: Could not open level definition file \"" << filename << "\"" << std::endl;
		return;
	}
	nlohmann::json json;
	is >> json;
	is.close();
	if (json.count("portals") > 0) {
		playingField->clear();
		if (json["portals"].count("good") > 0) {
			for (auto& portal : json["portals"]["good"]) {
				Cell* cell = &playingField->cells[portal["x"]][portal["y"]];
				cell->sporeType = SporeType::Good_Portal;
				cell->sporeSize = 1.0f;
			}
		}
		if (json["portals"].count("evil") > 0) {
			for (auto& portal : json["portals"]["evil"]) {
				Cell* cell = &playingField->cells[portal["x"]][portal["y"]];
				cell->sporeType = SporeType::Evil_Portal;
				cell->sporeSize = 1.0f;
			}
		}
	}
}

