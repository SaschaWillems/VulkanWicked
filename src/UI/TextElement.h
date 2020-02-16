/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <string>
#include <glm/glm.hpp>

namespace UI
{
    enum class TextAlignment {
        TopLeft,
        Center,
    };

    class TextElement
    {
    public:
        bool visible = true;
        TextAlignment alignment = TextAlignment::TopLeft;
        glm::vec3 position;
        glm::vec4 color;
        std::string text;
    };
}

