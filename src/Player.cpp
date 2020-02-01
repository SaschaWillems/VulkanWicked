/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "Player.h"

Player::Player() {
	health = 100.0f;
	position = glm::vec3(0.0f);
	velocity = glm::vec2(0.0f);
	rotation = glm::vec2(0.0f);
}

LightSource Player::getLightSource()
{
	// @todo: Change with player state like HP and bonuses
	LightSource lightSource;
	lightSource.position = glm::vec4(position.x, 1.4f, position.z, 1.0f);
	lightSource.color = glm::vec3(1.0f, 0.7f, 0.3f);
	lightSource.radius = 3.0f;
	return lightSource;
}

void Player::onKeyboardStateUpdated(const Uint8* keyboardState)
{
	// @todo: Make configurable, add gamepad support
	keys.up = keyboardState[SDL_SCANCODE_W];
	keys.down = keyboardState[SDL_SCANCODE_S];
	keys.left = keyboardState[SDL_SCANCODE_A];
	keys.right = keyboardState[SDL_SCANCODE_D];
}

void Player::onMouseButtonClick(uint32_t button)
{
	// @todo: Key bindings
	if (button == SDL_BUTTON_LEFT && firingCooldown <= 0.0f) {
		fireProjectile();
	}
	// @todo: Original game does shooting or picking up on one button
	if (button == SDL_BUTTON_RIGHT) {
		pickupObjects();
	}
}

void Player::fireProjectile()
{
	// Direction is the normalized vector between current player position and mouse cursor position
	// Mousepos needs to be mapped from window coordinate space to camera coordinate space
	const float w = gameState->boundingBox.width() * 0.5f;
	const float h = gameState->boundingBox.height() * 0.5f;
	glm::vec2 mposnc = (glm::vec2(mousePos) / gameState->windowSize) - glm::vec2(0.5f);
	mposnc *= glm::vec2(w, h) * 2.0f;
	const glm::vec2 ndir = glm::normalize(mposnc - glm::vec2(position.x, position.z));
	gameState->addProjectile(Projectile(position, glm::vec3(ndir.x, 0.0f, ndir.y), ProjectileType::Player));
	firingCooldown = gameState->values.playerFiringCooldown;
}

void Player::pickupObjects()
{
	switch (state) {
	case PlayerState::Default: {
		// Player can pick up good portal spawners and drop them onto full grown good spores to generate portals
		for (auto& projectile : gameState->projectiles) {
			if (projectile.type == ProjectileType::Good_Portal_Spawn) {
				if (projectile.distanceTo(position) < gameState->values.goodSpawnerProjectileSize) {
					std::clog << "Picked up portal spawner" << std::endl;
					state = PlayerState::Carries_Portal_Spawner;
					projectile.remove();
				}
			}
		}
		break;
	}
	case PlayerState::Carries_Portal_Spawner: {
		// Dropping a good portal spawner on a fully grown good spore turns it into a portal
		Cell* cell = playingField->cellFromVisualPos(position);
		if (cell && cell->sporeType == SporeType::Good && cell->sporeSize >= gameState->values.maxSporeSize) {
			cell->sporeType = SporeType::Good_Portal;
			state = PlayerState::Default;
		}
		break;
	}
	}
}

void Player::prepareGPUResources()
{
	VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ubo, sizeof(glm::mat4)));
	VK_CHECK_RESULT(ubo.map());

	descriptorSet = new DescriptorSet(renderer->device->handle);
	descriptorSet->setPool(renderer->descriptorPool);
	descriptorSet->addLayout(renderer->getDescriptorSetLayout("single_ubo"));
	descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &ubo.descriptor);
	descriptorSet->create();
}

void Player::updateGPUResources() {
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
	mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.copyTo(&mat, sizeof(mat));
}

void Player::update(float dT) {

	if (firingCooldown > 0.0f) {
		firingCooldown -= 0.1f;
	}

	glm::vec2 dir = glm::vec2(0.0f);
	if (keys.up) {
		dir.y = -1.0f;
	}
	if (keys.down) {
		dir.y = 1.0f;
	}
	if (keys.left) {
		dir.x = -1.0f;
	}
	if (keys.right) {
		dir.x = 1.0f;
	}

	if (glm::length(dir) != 0.0f) {
		glm::vec2 n = glm::normalize(dir);
		velocity += n * accelFactor * 0.05f;
	}

	if (glm::length(velocity) != 0.0f) {
		position.x += velocity.x;
		position.z += velocity.y;
		updateGPUResources();
	}

	if (position.x < gameState->boundingBox.left) {
		position.x = gameState->boundingBox.left;
	}
	if (position.x > gameState->boundingBox.right) {
		position.x = gameState->boundingBox.right;
	}
	if (position.z < gameState->boundingBox.top) {
		position.z = gameState->boundingBox.top;
	}
	if (position.z > gameState->boundingBox.bottom) {
		position.z = gameState->boundingBox.bottom;
	}

	velocity.x = velocity.x - velocity.x * dragFactor;
	velocity.y = velocity.y - velocity.y * dragFactor;

	rotation.y -= velocity.x * dT * 10.0f;
	if (rotation.y > 2.0f * M_PI) {
		rotation.y = rotation.y - (2.0f * M_PI);
	}
	if (rotation.y < -2.0f * M_PI) {
		rotation.y = rotation.y + (2.0f * M_PI);
	}
}

void Player::draw(CommandBuffer* cb)
{
	cb->bindPipeline(renderer->getPipeline("player"));
	cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, descriptorSet }, 0);
	assetManager->getModel("player_star")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
	if (state == PlayerState::Carries_Portal_Spawner) {
		assetManager->getModel("portal_spawner_good")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
	}
}
