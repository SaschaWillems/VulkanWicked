/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>
#include "Renderer/LightSource.h"

enum class ProjectileType {
	Player,
	Guardian,	
	Good_Portal_Spawn,
	Evil_Portal_Spawn
};

class Projectile
{
public:
	glm::vec3 pos;
	glm::vec3 dir;
	bool alive;
	int32_t intValue = 0;
	ProjectileType type;
	Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType type);
	float distanceTo(Projectile projectile);
	float distanceTo(glm::vec3 position);
	void remove();
	LightSource Projectile::getLightSource();
};

