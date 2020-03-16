/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>

#include "Renderer/RenderObject.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/LightSource.h"

#include "Utils.h"
#include "GameState.h"
#include "GameInputListener.h"
#include "PlayingField.h"
#include "Cell.h"
#include "Player.h"
#include "Guardian.h"
#include "UI/GameUI.h"

enum class View { None, Intro, MainMenu, LevelSelection, InGame, GameOver };

class Game: public GameInputListener, public RenderObject
{
private:
	void updateSpawnTimer(float dT);
	void updateState(float dT);
	void onKeyPress(SDL_Keycode keyCode);
public:
	View view = View::None;
	View targetView = View::None;
	float fade = 0.0f;
	Player* player;
	Guardian* guardian;
	DescriptorSet* descriptorSetProjectiles;
	bool paused = false;
	Buffer projectilesUbo;
	LightSource getPhaseLight();
	void spawnTrigger();
	void update(float dT);
	void updateProjectiles(float dT);
	void prepareGPUResources();
	void updateGPUResources();
	void spawnPlayer();
	void spawnGuardian();
	void setView(View view, bool fade = true);
	void loadLevel(const std::string& filename);
};

