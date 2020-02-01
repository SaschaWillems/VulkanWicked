/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Guardian.h"

Guardian::Guardian()
{
    health = 100.0f;
    position = glm::vec3(playingField->width / 2.0f - 1.0f, 0.0f, playingField->height / 2.0f - 1.0f);
    rotation = glm::vec2(0.0f);
    direction = glm::vec2(-1.0f, 0.0f);
}

LightSource Guardian::getLightSource()
{
    // @todo: Change with state like HP and bonuses
    LightSource lightSource;
    lightSource.position = glm::vec4(position.x, 1.4f, position.z, 1.0f);
    lightSource.color = glm::vec3(0.5f, 0.0f, 0.0f);
    lightSource.radius = 3.0f;
    return lightSource;
}

void Guardian::prepareGPUResources()
{
    VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ubo, sizeof(glm::mat4)));
    VK_CHECK_RESULT(ubo.map());

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
        position.x += n.x * 2.5f * dT;
        position.z += n.y * 2.5f * dT;

        if (direction.x < 0.0f && position.x < -(playingField->width / 2.0f) + 1.0f) {
            direction.x = 1.0f;
        }

        if (direction.x > 0.0f && position.x > (playingField->width / 2.0f) - 1.0f) {
            direction.x = -1.0f;
        }

        updateGPUResources();
    }
}

void Guardian::draw(CommandBuffer* cb)
{
    assert(model);
    //@todo: Distinct pipeline
    cb->bindPipeline(renderer->getPipeline("player"));
    cb->bindDescriptorSets(renderer->getPipelineLayout("split_ubo"), { renderer->descriptorSets.camera, descriptorSet }, 0);
    model->draw(cb->handle, renderer->getPipelineLayout("split_ubo")->handle);
}

bool Guardian::hitTest(glm::vec3 pos)
{
    // Add in some margin
    glm::vec2 hitSize = this->size * 0.9f;
    return (pos.x >= position.x - hitSize.x / 2.0f) && (pos.x <= position.x + hitSize.x / 2.0f) && (pos.z >= position.z - hitSize.y / 2.0f) && (pos.z <= position.z + hitSize.y / 2.0f);
}
