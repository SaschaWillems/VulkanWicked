/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "TarotDeck.h"

TarotDeck::TarotDeck()
{
	position = glm::vec3(0.0f);
	rotation = glm::vec3(0.0f, 0.0f, (float)M_PI * 0.5f);
	scale = glm::vec3(1.0f);
	stateTimer = 0.0f;
	activationTimer = 0.0f;
}

TarotDeck::~TarotDeck()
{
	ubo.destroy();
}

void TarotDeck::setState(TarotDeckState newState)
{
	state = newState;
	stateTimer = 0.0f;
	activationTimer = 0.0f;
	switch (newState) {
	case TarotDeckState::Appearing:
		rotation.z = (float)M_PI * 1.5f;
		break;
	case TarotDeckState::Visible:
		scale = glm::vec3(1.0f);
		position = glm::vec3(0.0f);
		rotation.z = (float)M_PI * 3.0f;
		break;
	case TarotDeckState::Disappearing:
		break;
	case TarotDeckState::Activated:
		rotation.y = 0.0f;
		break;
	}
}

void TarotDeck::prepareGPUResources()
{
	VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &ubo, sizeof(glm::mat4)));
	descriptorSets.resize(3);
	descriptorSets[0] = renderer->descriptorSets.camera;
	descriptorSets[1] = new DescriptorSet(renderer->device->handle);
	descriptorSets[1]->setPool(renderer->descriptorPool);
	descriptorSets[1]->addLayout(renderer->getDescriptorSetLayout("single_ubo"));
	descriptorSets[1]->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &ubo.descriptor);
	descriptorSets[1]->create();
	Texture* texture = assetManager->getTexture("tarot_deck_b");
	assert(texture);
	descriptorSets[2] = new DescriptorSet(renderer->device->handle);
	descriptorSets[2]->setPool(renderer->descriptorPool);
	descriptorSets[2]->addLayout(renderer->getDescriptorSetLayout("single_image"));
	descriptorSets[2]->addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texture->descriptor);
	descriptorSets[2]->create();
}

void TarotDeck::updateGPUResources()
{
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
	mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	mat = glm::rotate(mat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	mat = glm::scale(mat, scale);
	ubo.copyTo(&mat, sizeof(mat));
}

void TarotDeck::setModel(std::string name)
{
	model = assetManager->getModel(name);
	assert(model);
	size.x = model->dimensions.max.x - model->dimensions.min.x;
	size.y = model->dimensions.max.z - model->dimensions.min.z;
}

void TarotDeck::update(float dT, glm::vec3 playerPos)
{
	// Animation
	const float TWO_PI = 2.0f * (float)M_PI;
	switch (state) {
	case TarotDeckState::Appearing:
		// "Reveal" animation
		rotation.z += 0.75f * dT;
		if (rotation.z > 1.5f * TWO_PI) {
			setState(TarotDeckState::Visible);
		}
		break;
	case TarotDeckState::Disappearing:
		// Inverted "Reveal" animation
		rotation.z -= 1.0f * dT;
		if (rotation.z < -TWO_PI) {
			rotation.z += TWO_PI;
			setState(TarotDeckState::Hidden);
		}
		break;
	case TarotDeckState::Activated:
		// "Fall towars sun/moon" animation
		rotation.y += 4.0f * dT;
		if (rotation.y > TWO_PI) {
			rotation = glm::vec3(0.0f, 0.0f, (float)M_PI * 0.5f);
			scale = glm::vec3(1.0f);
			setState(TarotDeckState::Hidden);
		}
		scale = glm::vec3((TWO_PI - rotation.y) / TWO_PI);
		break;
	case TarotDeckState::Visible:
		// Player hit test
		// If tarot deck is visible, the player touching it will trigger it's effects
		// A timer is used to defer this a bit
		if (hitTest(playerPos)) {
			// Visual hint
			activationTimer += 1.0f * dT;
			if (activationTimer > 1.0f) {
				// @todo: Apply effect
				setState(TarotDeckState::Activated);
			}
		}
		else {
			if (activationTimer > 0.0f) {
				activationTimer -= 4.0f * dT;
				if (activationTimer < 0.0f) {
					activationTimer = 0.0f;
				}
			}
		}
		scale.x = 1.0f + sin(activationTimer * (float)M_PI) * 0.1f;
		scale.z = 1.0f + sin(activationTimer * (float)M_PI) * 0.1f;
		break;
	}
	updateGPUResources();
}

void TarotDeck::draw(CommandBuffer* cb)
{
	if (state != TarotDeckState::Hidden) {
		cb->bindPipeline(renderer->getPipeline("tarot_card"));
		cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo_single_image"), descriptorSets, 0);
		model->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
	}
}

bool TarotDeck::hitTest(glm::vec3 pos)
{
	if (state == TarotDeckState::Visible) {
		glm::vec2 hitSize = this->size * 0.9f;
		return (pos.x >= position.x - hitSize.x / 2.0f) && (pos.x <= position.x + hitSize.x / 2.0f) && (pos.z >= position.z - hitSize.y / 2.0f) && (pos.z <= position.z + hitSize.y / 2.0f);
	}
	return false;
}
