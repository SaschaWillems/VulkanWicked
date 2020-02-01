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

class Cell
{
public:
	float sporeSize = 0.0f;
	float floatValue = 0.0f;
	glm::vec2 gridPos;
	glm::vec2 rndOffset;
	SporeType sporeType = SporeType::Empty;
	bool empty();
	bool hasLightSource();
	LightSource getLightSource();
};

