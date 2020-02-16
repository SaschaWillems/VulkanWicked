/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <string>
#include <array>
#include <stdio.h>
#include "json.hpp"
#include "../Renderer/AssetManager.h"
#include "../Renderer/Device.h"
#include "../Renderer/Texture.h"
#include "../Renderer/DescriptorSet.h"

namespace UI
{

    struct FontCharInfo {
        float x, y;
        float width;
        float height;
        float xoffset;
        float yoffset;
        float xadvance;
    };

    class Font
    {
    public:
        std::string name;
        std::array<FontCharInfo, 255> charInfo;
        float lineHeight;
        Texture2D* texture;
        DescriptorSet* descriptorSet;
        void load(std::string fontname, Device* device, VkQueue transferQueue);
        uint32_t getWidth();
        uint32_t getHeight();
    };

}