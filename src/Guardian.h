/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>
#include "Renderer/RenderObject.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/LightSource.h"
#include "Renderer/VulkanglTFModel.h"
#include "GameState.h"
#include "PlayingField.h"

enum class GuardianState {
	Default,
};

class Guardian: public RenderObject
{
private:
	Buffer ubo;
	DescriptorSet* descriptorSet;
	vkglTF::Model* model;
public:
	glm::vec3 position;
	glm::vec2 direction;
	glm::vec2 rotation;
	glm::vec2 size;
	float health;
	GuardianState state = GuardianState::Default;
	Guardian();
	LightSource getLightSource();
	void prepareGPUResources();
	void updateGPUResources();
	void setModel(std::string name);
	void update(float dT);
	void draw(CommandBuffer* cb);
	bool hitTest(glm::vec3 pos);
};

