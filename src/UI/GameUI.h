/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <unordered_map> 
#include "../Renderer/RenderObject.h"
#include "../Renderer/VulkanTools.h"
#include "../Renderer/Buffer.h"
#include "../Renderer/CommandBuffer.h"
#include "Font.h"
#include "TextElement.h"

namespace UI
{

    class GameUI : public RenderObject
    {
    private:
        struct Vertex {
            glm::vec3 pos;
            glm::vec2 uv;
            glm::vec4 color;
        };
        std::vector<Vertex> vertices;
        Buffer vertexBuffer;
        std::unordered_map<std::string, Font*> fonts;
        Font* font = nullptr;
        std::unordered_map<std::string, TextElement*> textElements;
    public:
        ~GameUI();
        void addFont(std::string name);
        void setfont(std::string name);
        void addTextElement(std::string name, std::string text, glm::vec3 position, TextAlignment alignment, glm::vec4 color = glm::vec4(1.0f), bool visible = true);
        TextElement* getTextElement(std::string name);
        void prepareGPUResources();
        void updateGPUResources();
        void draw(CommandBuffer* cb);
    };

}

extern UI::GameUI* gameUI;
