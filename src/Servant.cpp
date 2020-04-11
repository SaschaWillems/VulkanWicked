/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Servant.h"

Servant::Servant()
{
    health = 100.0f;
    position = glm::vec3(0.0f);
    rotation = glm::vec2(0.0f);
    direction = glm::vec2(0.0f);
}

LightSource Servant::getLightSource()
{
    // @todo: Change with state like HP and bonuses
    LightSource lightSource;
    lightSource.position = glm::vec4(position.x, zIndex, position.z, 1.0f);
    lightSource.color = glm::vec3(1.0f);
    lightSource.radius = alive() ? 2.0f : 0.0f;
    if (state == ServantState::Appearing) {
        lightSource.color *= stateTimer;
    }
    if (state == ServantState::Disappearing) {
        lightSource.color *= 1.0f - stateTimer;
    }
    return lightSource;
}

void Servant::prepareGPUResources()
{
    VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &ubo, sizeof(glm::mat4)));
    descriptorSet = new DescriptorSet(renderer->device->handle);
    descriptorSet->setPool(renderer->descriptorPool);
    descriptorSet->addLayout(renderer->getDescriptorSetLayout("single_ubo"));
    descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &ubo.descriptor);
    descriptorSet->create();
}

void Servant::updateGPUResources()
{
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    if (state == ServantState::Appearing) {
        mat = glm::scale(mat, glm::vec3(stateTimer));
    }
    if (state == ServantState::Disappearing) {
        mat = glm::scale(mat, glm::vec3(1.0f - stateTimer));
    }
    ubo.copyTo(&mat, sizeof(mat));
}

void Servant::setModel(std::string name)
{
    model = assetManager->getModel(name);
    assert(model);
    size.x = model->dimensions.max.x - model->dimensions.min.x;
    size.y = model->dimensions.max.z - model->dimensions.min.z;
}

void Servant::changeDirection()
{
    direction.x = randomFloat(2.0f) - 1.0f;
    direction.y = randomFloat(2.0f) - 1.0f;
}

void Servant::update(float dT)
{
    // @todo: Different and proper movement behaviours

    stateTimer += 1.0f * dT;
    switch (state) {
    case ServantState::Appearing:
        if (stateTimer >= 1.0f) {
            state = ServantState::Alive;
            stateTimer = 0.0f;
        }
        break;
    case ServantState::Alive:
        if (stateTimer >= gameState->values.servantLifespan) {
            state = ServantState::Disappearing;
            stateTimer = 0.0f;
        }
        break;
    case ServantState::Disappearing:
        if (stateTimer >= 1.0f) {
            state = ServantState::Dead;
            stateTimer = 0.0f;
            return;
        }
        break;
    }

    // Randomly change direciton
    directionChangeTimer -= 0.5f * dT;
    if (directionChangeTimer <= 0.0f) {
        changeDirection();
        directionChangeTimer = 1.0f;// +randomFloat(1.0f);
    }

    if (glm::length(direction) != 0.0f) {
        glm::vec2 n = glm::normalize(direction);
        velocity += n * accelFactor * dT;
    }

    if (glm::length(velocity) != 0.0f) {
        position.x += velocity.x * dT;
        position.z += velocity.y * dT;
    }

    // Switch velocity at playing field borders
    const float margin = 0.75f;
    if (position.x <= gameState->boundingBox.left * margin && direction.x < 0.0f) {
        direction.x = 1.0f;
    }
    if (position.x >= gameState->boundingBox.right * margin && direction.x > 0.0f) {
        direction.x = -1.0f;
    }
    if (position.z <= gameState->boundingBox.top * margin && direction.y < 0.0f) {
        direction.y = 1.0f;
    }
    if (position.z >= gameState->boundingBox.bottom * margin && direction.y > 0.0f) {
        direction.y = -1.0f;
    }

    // Clamp to borders
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

    velocity.x = velocity.x - velocity.x * (dragFactor * dT);
    velocity.y = velocity.y - velocity.y * (dragFactor * dT);

    rotation.y += rotationDir * dT;
    if (rotation.y > 2.0f * (float)M_PI) {
        rotation.y -= 2.0f * (float)M_PI;
    }

    updateGPUResources();
}

void Servant::draw(CommandBuffer* cb)
{
    assert(model);
    //@todo: Distinct pipeline
    if (alive()) {
        cb->bindPipeline(renderer->getPipeline("player"));
        cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, descriptorSet }, 0);
        model->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
    }
}

void Servant::spawn(glm::vec2 spawnPosition)
{
    state = ServantState::Appearing;
    health = 100.0f;
    position = glm::vec3(spawnPosition.x, -zIndex, spawnPosition.y);
    rotation = glm::vec2(0.0f);
    velocity = glm::vec2(0.0f);
    directionChangeTimer = 2.5f;
    rotationDir = randomFloat(1.0f) < 0.5f ? -1.0f : 1.0f;
}

bool Servant::hitTest(glm::vec3 pos)
{
    // Add in some margin
    glm::vec2 hitSize = this->size * 0.9f;
    return (pos.x >= position.x - hitSize.x / 2.0f) && (pos.x <= position.x + hitSize.x / 2.0f) && (pos.z >= position.z - hitSize.y / 2.0f) && (pos.z <= position.z + hitSize.y / 2.0f);
}

bool Servant::alive()
{
    return state != ServantState::Dead;
}

void Servant::remove()
{
    state = ServantState::Disappearing;
    stateTimer = 0.0f;
}
