/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include "Renderer/LightSource.h"

enum class SporeType {
	Empty,
	Good,
	Good_Portal,
	Evil,
	Evil_Portal,
	Evil_Dead,
	Deadzone
};

class SporeSize {
public:
	static const float None;
	static const float Small;
	static const float Medium;
	static const float Max;
};

class Cell
{
public:
	// Spores are "owned" by the portal they're spawned from
	// This is required for growth calculations
	Cell* owner = nullptr;
	// Store neighbours for performance
	Cell* neighbours[8];
	float sporeSize = 0.0f;
	float floatValue = 0.0f;
	float portalGrowTimer = 1.0f;
	glm::ivec2 pos;
	glm::vec2 gridPos;
	glm::vec2 rndOffset;
	float zIndex = 0.0f;
	SporeType sporeType = SporeType::Empty;
	bool empty();
	bool hasLightSource();
	LightSource getLightSource();
	void grow();
	bool canGrow();
	uint32_t getNeighbourCount(SporeType sporeType, Cell* owner);
	float getNewZIndexFromNeighbours();
};

