/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "SDL.h"
#include "GameInputListener.h"

class GameInput
{
private:
	const Uint8* keyboardState;
	std::vector<GameInputListener*> inputListeners;
public:
	struct Mouse {
		glm::ivec2 position;
		struct Buttons {
			bool left;
			bool middle;
			bool right;
		} buttons;
	} mouse;
	void update();
	void handleInput(SDL_Event& event);
	void addInputListener(GameInputListener* listener);
};

extern GameInput* input;