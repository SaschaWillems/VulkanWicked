/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GameInput.h"

GameInput* input = nullptr;

void GameInput::update()
{
	int mx;
	int my;
	SDL_GetMouseState(&mx, &my);
	mouse.position = glm::vec2(mx, my);

	keyboardState = SDL_GetKeyboardState(nullptr);
	for (auto listener : inputListeners) {
		listener->onKeyboardStateUpdated(keyboardState);
	}
	for (auto listener : inputListeners) {
		listener->mousePos = glm::ivec2(mx, my);
	}
}

void GameInput::handleInput(SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_KEYDOWN:
		if (event.key.repeat == 0) {
			for (auto listener : inputListeners) {
				listener->onKeyPress(event.key.keysym.sym);
			}
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == SDL_BUTTON_LEFT) {
			if (!mouse.buttons.left) {
				for (auto listener : inputListeners) {
					listener->onMouseButtonClick(SDL_BUTTON_LEFT);
				}
			}
			mouse.buttons.left = true;
		}
		if (event.button.button == SDL_BUTTON_RIGHT) {
			if (!mouse.buttons.right) {
				for (auto listener : inputListeners) {
					listener->onMouseButtonClick(SDL_BUTTON_RIGHT);
				}
			}
			mouse.buttons.right = true;
		}
		if (event.button.button == SDL_BUTTON_MIDDLE) {
			if (!mouse.buttons.middle) {
				for (auto listener : inputListeners) {
					listener->onMouseButtonClick(SDL_BUTTON_MIDDLE);
				}
			}
			mouse.buttons.middle = true;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if (event.button.button == SDL_BUTTON_LEFT) {
			mouse.buttons.left = false;
		}
		if (event.button.button == SDL_BUTTON_RIGHT) {
			mouse.buttons.right = false;
		}
		if (event.button.button == SDL_BUTTON_MIDDLE) {
			mouse.buttons.middle = false;
		}
		break;
	case SDL_MOUSEMOTION:
		// @todo
		//handleMouseMove(sdlEvent.motion.x, sdlEvent.motion.y);
		break;
	}
}

void GameInput::addInputListener(GameInputListener* listener)
{
	inputListeners.push_back(listener);
}
