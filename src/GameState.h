/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#pragma once

#include <vector>
#include <iostream>
#include "BoundingBox.h"
#include "Projectile.h"

enum class Phase { Day, Night };

class GameStateValues 
{
public:
	const int maxNumProjectiles = 512;
	float playingFieldDeadzone = 4.5f;
	float maxSporeSize = 0.75f;
	float maxGrowthDistanceToPortal = 5.0f;
	float maxNumGoodPortalSpawners = 2;
	float maxNumEvilPortalSpawners = 3;
	float growthSpeedFast = 0.25f;
	float growthSpeedSlow = 0.125f;
	float phaseDuration = 15.0f;
	float spawnTimer = 1.0f;
	float goodSpawnerProjectileSize = 0.25f;
	float evilPortalSpawnerSpawnChance = 0.15f;
	float evilPortalSpawnerSpeed = 12.5f;
	float evilSpawnerProjectileSize = 0.25f;
	float evilDeadSporeLife = 1.0f;
	float evilDeadSporeRessurectionSpeed = 0.5f;
	float playerFiringCooldown = 2.0f;
	float playerProjectileSpeed = 17.0f;
	float playerProjectileGuardianDamage = 5.0f;
	//
	float portalGrowthSpeedFast = 0.5f * 10.0f;
	float portalGrowthSpeedSlow = 0.075f * 10.0f;
	float portalGrowTimer = 1.0f;
	// Can be changed to adjust difficulty
	float portalGrowthFactorGood = 1.0f;
	float portalGrowthFactorEvil = 1.0f;
};

class GameState
{
public:
	GameStateValues values;
	Phase phase;
	float phaseTimer;
	float spawnTimer;
	std::vector<Projectile> projectiles;
	glm::vec2 windowSize;
	BoundingBox boundingBox;
	GameState();
	void addProjectile(Projectile projectile);
	uint32_t projectileCountByType(ProjectileType type);
	void clear();
};

extern GameState* gameState;
