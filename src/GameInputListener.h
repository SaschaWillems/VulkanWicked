/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include "SDL.h"

class GameInputListener
{
public:
	glm::ivec2 mousePos;
	virtual void onMouseButtonClick(uint32_t button) {};
	virtual void onKeyPress(SDL_Keycode keyCode) {};
	virtual void onKeyboardStateUpdated(const uint8_t* keyboardState) {};
};

