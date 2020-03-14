/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>
#include "Renderer/LightSource.h"

enum class SporeType {
	Empty,
	Good,
	Good_Portal,
	Evil,
	Evil_Portal,
	Evil_Dead
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
	float sporeSize = 0.0f;
	float floatValue = 0.0f;
	float portalGrowTimer = 1.0f;
	glm::vec2 gridPos;
	glm::vec2 rndOffset;
	SporeType sporeType = SporeType::Empty;
	bool empty();
	bool hasLightSource();
	LightSource getLightSource();
	void grow();
	bool canGrow();
};

