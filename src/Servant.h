/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <glm/glm.hpp>
#include "Renderer/RenderObject.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/LightSource.h"
#include "Renderer/VulkanglTFModel.h"
#include "Utils.h"
#include "GameState.h"
#include "PlayingField.h"

enum class ServantState {
	Appearing,
	Alive,
	Dead,
	Disappearing
};

#pragma once
class Servant: public RenderObject
{
private:
	Buffer ubo;
	DescriptorSet* descriptorSet;
	vkglTF::Model* model;
	void changeDirection();
public:
	float zIndex = 255.0f;
	glm::vec3 position;
	glm::vec2 direction;
	glm::vec2 velocity;
	glm::vec2 rotation;
	glm::vec2 size;
	float rotationDir = 0.0f;
	const float accelFactor = 12.5f;
	const float dragFactor = 1.0f;
	const float lifespan = 5.0f;
	float health;
	float directionChangeTimer = 1.0f;
	float stateTimer = 0.0f;
	ServantState state = ServantState::Appearing;
	Servant();
	LightSource getLightSource();
	void prepareGPUResources();
	void updateGPUResources();
	void setModel(std::string name);
	void update(float dT);
	void draw(CommandBuffer* cb);
	void spawn(glm::vec2 spawnPosition);
	bool hitTest(glm::vec3 pos);
	bool alive();
};

