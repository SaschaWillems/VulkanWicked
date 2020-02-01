/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "Projectile.h"

Projectile::Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType type)
{
	this->pos = pos;
	this->dir = dir;
	this->alive = true;
	this->type = type;
}

float Projectile::distanceTo(Projectile projectile)
{
	const uint32_t dx = pos.x - projectile.pos.x;
	const uint32_t dy = pos.z - projectile.pos.z;
	return sqrt(dx * dx + dy * dy);
}

float Projectile::distanceTo(glm::vec3 position)
{
	const uint32_t dx = pos.x - position.x;
	const uint32_t dy = pos.z - position.z;
	return sqrt(dx * dx + dy * dy);
}

void Projectile::remove()
{
	alive = false;
	pos = glm::vec3(-999.0f);
}

LightSource Projectile::getLightSource()
{
	LightSource lightSource;
	lightSource.position = glm::vec4(pos.x, 0.1f, pos.z, 1.0f);
	switch (type) {
	case ProjectileType::Player:
		lightSource.color = glm::vec3(0.75f);
		lightSource.radius = 2.5f;
		break;
	case ProjectileType::Good_Portal_Spawn:
		lightSource.color = glm::vec3(0.0f);
		lightSource.radius = 2.5f;
		break;
	case ProjectileType::Evil_Portal_Spawn:
		lightSource.color = glm::vec3(0.75f, 0.0f, 0.0f);
		lightSource.radius = 3.0f;
		break;
	}
	return lightSource;
}
