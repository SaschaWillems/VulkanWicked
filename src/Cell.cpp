/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Cell.h"

const float SporeSize::None = 0.0f;
const float SporeSize::Small = 0.5f;
const float SporeSize::Medium = 0.75f;
const float SporeSize::Max = 0.95f;

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
	lightSource.position = glm::vec4(gridPos.x + rndOffset.x, -zIndex-0.1f, gridPos.y + rndOffset.y, 0.0f);
	lightSource.radius = 2.0f;
	switch(sporeType) {
	case SporeType::Good_Portal:
//		lightSource.color = glm::vec3(1.0f, 0.7f, 0.3f);
		lightSource.color = glm::vec3(0.25f);
		lightSource.radius = 4.0f;
		break;
	case SporeType::Evil_Portal:
		//lightSource.color = glm::vec3(0.0f, 0.0f, 1.0f);
		lightSource.color = glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	}
	return lightSource;
}

void Cell::grow()
{
	if (sporeSize == SporeSize::None) {
		sporeSize = SporeSize::Small;
		return;
	}
	if (sporeSize == SporeSize::Small) {
		sporeSize = SporeSize::Medium;
		return;
	}
	if (sporeSize == SporeSize::Medium) {
		sporeSize = SporeSize::Max;
		return;
	}
}

bool Cell::canGrow()
{
	return (sporeSize < SporeSize::Max);
}

uint32_t Cell::getNeighbourCount(SporeType sporeType, Cell* owner)
{
	uint32_t count = 0;
	for (Cell* neighbour : neighbours) {
		if ((neighbour) && (neighbour->sporeType == sporeType) && (neighbour->owner = owner)) {
			count++;
		}
	}
	return count;
}

float Cell::getNewZIndexFromNeighbours()
{
	float maxZ = zIndex;
	for (Cell* neighbour : neighbours) {
		if ((neighbour) && (neighbour->zIndex > maxZ)) {
			maxZ = neighbour->zIndex;
		}
	}
	return std::min(maxZ + 0.1f, 256.0f);
}

