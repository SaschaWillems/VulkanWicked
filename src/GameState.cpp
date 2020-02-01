/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GameState.h"

GameState* gameState = nullptr;

GameState::GameState()
{
	// @todo
	phase = Phase::Day;
	phaseTimer = values.phaseDuration;
	spawnTimer = values.spawnTimer;
}

void GameState::addProjectile(Projectile projectile)
{
	// Try to reuse dead particles before increasing vector size
	// @todo: maybe resize in chunks for perf?
	int32_t reuseIdx = -1;
	for (auto i = 0; i < projectiles.size(); i++) {
		if (!projectiles[i].alive) {
			reuseIdx = i;
			break;
		}
	}
	if (reuseIdx > -1) {
		projectiles[reuseIdx] = projectile;
	}
	else {
		if (projectiles.size() < values.maxNumProjectiles) {
			projectiles.push_back(projectile);
		}
		else {
			std::cout << "Max. num of projectiles reached!" << std::endl;
		}
	}
}

uint32_t GameState::projectileCountByType(ProjectileType type)
{
	uint32_t count = 0;
	for (auto projectile : projectiles) {
		if (projectile.type == type)
			count++;
	}
	return count;
}
