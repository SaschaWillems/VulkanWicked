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
}

TarotDeck::~TarotDeck()
{
	ubo.destroy();
}

void TarotDeck::setState(TarotDeckState newState)
{
	if (newState == TarotDeckState::Appearing) {
		rotation.z = (float)M_PI * 0.5f;
	}
	state = newState;
	stateTimer = 0.0f;
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

	Texture* texture = assetManager->getTexture("tarot_deck");
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
	ubo.copyTo(&mat, sizeof(mat));
}

void TarotDeck::update(float dT)
{
	switch (state) {
	case TarotDeckState::Appearing:
		rotation.z += 1.0f * dT;
		if (rotation.z > 2.0f * (float)M_PI) {
			rotation.z -= 2.0f * (float)M_PI;
			state = TarotDeckState::Visible;
		}
	}
	updateGPUResources();
}

void TarotDeck::draw(CommandBuffer* cb)
{
	if (state != TarotDeckState::Hidden) {
		cb->bindPipeline(renderer->getPipeline("tarot_card"));
		cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo_single_image"), descriptorSets, 0);
		assetManager->getModel("tarot_card")->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
	}
}
