/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "Renderer/CommandBuffer.h"
#include "Renderer/AssetManager.h"
#include "Renderer/Buffer.h"
#include "Renderer/RenderObject.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/LightSource.h"
#include "GameState.h"
#include "GameInputListener.h"
#include "PlayingField.h"

#pragma once

enum class PlayerState {
	Default,
	Carries_Portal_Spawner,
};

class Player: public GameInputListener, public RenderObject
{
private:
	float firingCooldown = 0.0f;
	void onKeyboardStateUpdated(const Uint8* keyboardState);
	void onMouseButtonClick(uint32_t button);
	void fireProjectile();
	void pickupObjects();
public:
	Buffer ubo;
	DescriptorSet* descriptorSet;
	glm::vec3 position;
	float health;
	PlayerState state = PlayerState::Default;
	const float accelFactor = 0.2f;
	const float dragFactor = 0.02f;
	glm::vec2 velocity;
	glm::vec2 rotation;
	struct {
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;
	Player();
	LightSource getLightSource();
	void prepareGPUResources();
	void updateGPUResources();
	void update(float dT);
	void draw(CommandBuffer* cb);
	void spawn(glm::vec2 spawnPosition);
};

