/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>
#include "Renderer/RenderObject.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Texture.h"
#include "Renderer/VulkanglTFModel.h"

enum class TarotDeckState {
    Hidden,
    Appearing,
    Visible,
    Disappearing,
    Activated
};

class TarotDeck: public RenderObject
{
private:
    vkglTF::Model* model;
public:
    TarotDeckState state = TarotDeckState::Hidden;
    float stateTimer;
    float activationTimer;
    Buffer ubo;
    std::vector<DescriptorSet*> descriptorSets;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec2 size;
    TarotDeck();
    ~TarotDeck();
    void setState(TarotDeckState newState);
    void prepareGPUResources();
    void updateGPUResources();
    void setModel(std::string name);
    void update(float dT, glm::vec3 playerPos);
    void draw(CommandBuffer* cb);
    bool hitTest(glm::vec3 pos);
};

