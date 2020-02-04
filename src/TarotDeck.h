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

enum class TarotDeckState {
    Hidden,
    Appearing,
    Visible,
    Disappearing
};

class TarotDeck: public RenderObject
{
public:
    TarotDeckState state = TarotDeckState::Hidden;
    float stateTimer;
    Buffer ubo;
    std::vector<DescriptorSet*> descriptorSets;
    glm::vec3 position;
    glm::vec3 rotation;
    TarotDeck();
    ~TarotDeck();
    void setState(TarotDeckState newState);
    void prepareGPUResources();
    void updateGPUResources();
    void update(float dT);
    void draw(CommandBuffer* cb);
};

