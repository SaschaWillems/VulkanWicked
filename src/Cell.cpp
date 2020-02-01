/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Cell.h"

bool Cell::empty() {
	return (sporeType == SporeType::Empty);
}

bool Cell::hasLightSource()
{
	return (sporeType == SporeType::Good_Portal || sporeType == SporeType::Evil_Portal);
}

LightSource Cell::getLightSource()
{
	LightSource lightSource;
	lightSource.position = glm::vec4(gridPos.x + rndOffset.x, 0.5f, gridPos.y + rndOffset.y, 0.0f);
	lightSource.radius = 4.0f;
	switch(sporeType) {
	case SporeType::Good_Portal:
		lightSource.color = glm::vec3(1.0f, 0.7f, 0.3f);
		break;
	case SporeType::Evil_Portal:
		lightSource.color = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	}
	return lightSource;
}
