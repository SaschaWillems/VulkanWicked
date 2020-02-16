/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Font.h"

namespace UI
{
	void Font::load(std::string fontname, Device* device, VkQueue transferQueue)
	{
		std::ifstream is(assetManager->assetPath + "fonts/" + fontname + ".json");
		if (!is.is_open()) {
			std::cerr << "Error: Could not open font definition file for \"" << fontname << "\"" << std::endl;
			return;
		}
		// Load font definition from json
		nlohmann::json json;
		is >> json;
		is.close();
		for (auto& charinfo : json["chars"]) {
			FontCharInfo& ci = charInfo[charinfo["id"]];
			ci.x = charinfo["x"];
			ci.y = charinfo["y"];
			ci.width = charinfo["width"];
			ci.height = charinfo["height"];
			ci.xoffset = charinfo["xoffset"];
			ci.yoffset = charinfo["yoffset"];
			ci.xadvance = charinfo["xadvance"];
		}
		lineHeight = json["common"]["lineHeight"];
		texture = new Texture2D();
		texture->loadFromFile(assetManager->assetPath + "fonts/" + fontname + ".ktx", device, transferQueue);
	}

	uint32_t Font::getWidth()
	{
		assert(texture);
		return texture->width;
	}

	uint32_t Font::getHeight()
	{
		assert(texture);
		return texture->height;
	}
}