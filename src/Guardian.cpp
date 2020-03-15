/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Guardian.h"

Guardian::Guardian()
{
    health = 100.0f;
    position = glm::vec3(0.0f);
    rotation = glm::vec2(0.0f);
    direction = glm::vec2(0.0f);
}

LightSource Guardian::getLightSource()
{
    // @todo: Change with state like HP and bonuses
    LightSource lightSource;
    lightSource.position = glm::vec4(position.x, zIndex, position.z, 1.0f);
    lightSource.color = glm::vec3(0.5f, 0.0f, 0.0f);
    lightSource.radius = alive() ? 3.0f : 0.0f;
    return lightSource;
}

void Guardian::prepareGPUResources()
{
    VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &ubo, sizeof(glm::mat4)));
    descriptorSet = new DescriptorSet(renderer->device->handle);
    descriptorSet->setPool(renderer->descriptorPool);
    descriptorSet->addLayout(renderer->getDescriptorSetLayout("single_ubo"));
    descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &ubo.descriptor);
    descriptorSet->create();
}

void Guardian::updateGPUResources()
{
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.copyTo(&mat, sizeof(mat));
}

void Guardian::setModel(std::string name)
{
    model = assetManager->getModel(name);
    assert(model);
    size.x = model->dimensions.max.x - model->dimensions.min.x;
    size.y = model->dimensions.max.z - model->dimensions.min.z;
}

void Guardian::update(float dT)
{
    // @todo: Different and proper movement behaviours
    if (glm::length(direction) != 0.0f) {
        glm::vec2 n = glm::normalize(direction);
        position.x += n.x * 5.0f * dT;
        position.z += n.y * 5.0f * dT;
        if ((position.x <= gameState->boundingBox.left && direction.x < 0.0f) || (position.x >= gameState->boundingBox.right && direction.x > 0.0f)) {
            direction.x = -direction.x;
        }
        if ((position.z <= gameState->boundingBox.top && direction.y < 0.0f) || (position.z >= gameState->boundingBox.bottom && direction.y > 0.0f)) {
            direction.y = -direction.y;
        }
        updateGPUResources();
    }
}

void Guardian::draw(CommandBuffer* cb)
{
    assert(model);
    //@todo: Distinct pipeline
    if (alive()) {
        cb->bindPipeline(renderer->getPipeline("player"));
        cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, descriptorSet }, 0);
        model->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
    }
}

void Guardian::spawn(glm::vec2 spawnPosition)
{
    state = GuardianState::Default;
    health = 100.0f;
    position = glm::vec3(spawnPosition.x, -zIndex, spawnPosition.y);
    rotation = glm::vec2(0.0f);
    // @todo: Proper random direction
    direction.x = randomFloat(1.0f) < 0.5f ? -1.0f : 1.0f;
    direction.y = randomFloat(1.0f) < 0.5f ? -1.0f : 1.0f;
}

bool Guardian::hitTest(glm::vec3 pos)
{
    // Add in some margin
    glm::vec2 hitSize = this->size * 0.9f;
    return (pos.x >= position.x - hitSize.x / 2.0f) && (pos.x <= position.x + hitSize.x / 2.0f) && (pos.z >= position.z - hitSize.y / 2.0f) && (pos.z <= position.z + hitSize.y / 2.0f);
}

bool Guardian::alive()
{
    return (health > 0.0f);
}
