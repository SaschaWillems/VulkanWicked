/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "PlayingField.h"

PlayingField* playingField = nullptr;

void PlayingField::generate(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;
	cells.resize(width);
	for (uint32_t i = 0; i < cells.size(); i++) {
		cells[i].resize(height);
	}

	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndFloat(-0.1f, 0.1f);

	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			cell.sporeType = SporeType::Empty;
			cell.sporeSize = 0.0f;
			cell.pos = glm::ivec2(x, y);
			cell.gridPos = glm::vec2(-(width * gridSize / 2.0f) + x * gridSize + gridSize / 2.0f, -(height * gridSize / 2.0f) + y * gridSize + gridSize / 2.0f);
			cell.rndOffset = glm::vec2(rndFloat(rndEngine), rndFloat(rndEngine));
			cell.rndOffset = glm::vec2(0.0f);
		}
	}

	// Pre-built some structures to save compute time
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			if (deadZone(x, y)) {
				cell.sporeType = SporeType::Deadzone;
			}
			// Get direct neighbours (no diagonal!)
			uint32_t neighbourIndex = 0;
			const glm::ivec2 offsets[4] = { {0, -1}, {0, 1}, {-1,  0}, {1,  1} };
			for (auto offset : offsets) {
				glm::ivec2 dst = glm::vec2(x + offset.x, y + offset.y);
				cell.neighbours[neighbourIndex] = cellAt(dst);
				neighbourIndex++;
			}
		}
	}
}

void PlayingField::clear()
{
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			cell.sporeType = SporeType::Empty;
			cell.sporeSize = 0.0f;
			if (deadZone(x, y)) {
				cell.sporeType = SporeType::Deadzone;
			}
		}
	}
}

void PlayingField::updateSpore(Cell& cell, float dT)
{
	if (cell.sporeType == SporeType::Evil_Dead) {
		cell.floatValue -= dT * gameState->values.evilDeadSporeRessurectionSpeed;
		if (cell.floatValue <= 0) {
			// Dead evil spores return to live after a certain amount of time and if not overtaken by good growth
			cell.sporeType = SporeType::Evil;
		}
	}
}

void PlayingField::updatePortal(Cell* portal, float dT)
{
	// Reworked growth functionality based around portals (seems to be what the original is doing)
	// If growth timer triggers, either spawn a new small spore in portals' range or grow an existing one
	// Spawning is done in growing distance around the portal, once all spores at a given distance are maxed out, it's possible for spores to spawn further away
	// @todo: Add some more randomization
	// @todo: Cell to grow mus be reachable by portal!
	// @todo: Overtaking other type cells in here
	// @todo: Portal growth at playing field border not properly working
	// Portal growth timer speed depends on day/night cycle
	SporeType sporeType;
	SporeType otherPortalType;
	switch (portal->sporeType) {
	case SporeType::Good_Portal:
		portal->portalGrowTimer -= dT * ((gameState->phase == Phase::Day) ? gameState->values.portalGrowthSpeedFast : gameState->values.portalGrowthSpeedSlow) * gameState->values.portalGrowthFactorGood;
		sporeType = SporeType::Good;
		otherPortalType = SporeType::Evil_Portal;
		break;
	case SporeType::Evil_Portal:
		portal->portalGrowTimer -= dT * ((gameState->phase == Phase::Night) ? gameState->values.portalGrowthSpeedFast : gameState->values.portalGrowthSpeedSlow) * gameState->values.portalGrowthFactorEvil;
		sporeType = SporeType::Evil;
		otherPortalType = SporeType::Good_Portal;
		break;
	}
	if (portal->portalGrowTimer > 0.0f) {
		return;
	}
	// Growth calculations
	// @todo: Add randomization;
	portal->portalGrowTimer = gameState->values.portalGrowTimer;
	const int32_t maxDist = 4;
	int32_t currentDist = 1;
	//std::vector<Cell*> cells;
	Cell* cells[(maxDist*2) * (maxDist*2)];
	uint32_t cellCount;
	// @todo: slow down growth chance based on current distance (1 = 100%, 2 = 50%, 3 = 25%, etc.)
	while (true) {
		// Check if there is any cell at the current distance from the portal that's not fully grown
		cellCount = 0;
		for (int32_t x = portal->pos.x - currentDist; x <= portal->pos.x + currentDist; x++) {
			for (int32_t y = portal->pos.y - currentDist; y <= portal->pos.y + currentDist; y++) {
				if ((x > -1) && (x < width) && (y > -1) && (y < height)) {
					if (std::max(abs(x - portal->pos.x), abs(y - portal->pos.y)) == currentDist) {
						Cell* cell = &this->cells[x][y];
						if (cell->sporeType == SporeType::Deadzone) {
							continue;
						}
						if (cell->sporeSize >= SporeSize::Max) {
							continue;
						}
						// Skip temporary disabled/dead evil cells for evil portals
						if (portal->sporeType == SporeType::Evil_Portal && cell->sporeType == SporeType::Evil_Dead) {
							continue;
						}
						// Check if cell can be reached from portal
						if (currentDist > 1) {
							if (cell->getNeighbourCount(sporeType, portal) == 0) {
								continue;
							}
						}
						if (cell) {
							cells[cellCount] = cell;
							cellCount++;
						}
					}
				}
			}
		}

		// @todo: also take portals into account
		bool skip = true;
		for (uint32_t i = 0; i < cellCount; i++) {
			//if (dstCell && ((dstCell->sporeType == SporeType::Empty || dstCell->sporeSize < SporeSize::Max)) || (dstCell->sporeType == otherPortalType)) {
			if (cells[i] && ((cells[i]->sporeType == SporeType::Empty || cells[i]->sporeSize < SporeSize::Max))) {
				skip = false;
				break;
			}
		}
		// Skip to next distance
		if (skip) {
			currentDist++;
			if (currentDist > maxDist) {
				break;
			}
			continue;
		}
		// Grow random cell
		// Growth chance decreases with distance
		float growthChance = std::max(100.0f - ((currentDist - 1) * 50.0f), 1.0f);
		if (randomFloat(100.0f) > growthChance) {
			continue;
		}
		Cell* dstCell = cells[randomInt(cellCount)];
		if (dstCell->sporeType == SporeType::Empty) {
			// @todo: remove owner on takeover!
			dstCell->owner = portal;
			dstCell->sporeType = sporeType;
			dstCell->sporeSize = SporeSize::Small;
			dstCell->zIndex = dstCell->getNewZIndexFromNeighbours();
			break;
		}
		else if (dstCell->sporeType == otherPortalType) {
			// Portals can be overgrown 
			// @todo: not working as intended
			/*
			dstCell->sporeType = otherPortalType == SporeType::Evil_Portal ? SporeType::Good : SporeType::Evil;
			dstCell->sporeSize = SporeSize::Small;
			dstCell->zIndex = getMaxCellZIndex(dstCell->pos);
			std::clog << (otherPortalType == SporeType::Evil_Portal ? "Good" : "Evil") << " portal owergrown" << std::endl;
			*/
			break;
		}
		else {
			if (dstCell->canGrow()) {
				dstCell->grow();
				// Bring forward
				dstCell->zIndex = dstCell->getNewZIndexFromNeighbours();
				break;
			}
		}
	}

}

void PlayingField::update(float dT)
{
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			switch (cell.sporeType) {
			case SporeType::Good:
			case SporeType::Evil:
			case SporeType::Evil_Dead:
				updateSpore(cell, dT);
				break;
			case SporeType::Good_Portal:
			case SporeType::Evil_Portal:
				updatePortal(&cell, dT);
				break;
			}
		}
	}
}

bool PlayingField::deadZone(uint32_t x, uint32_t y)
{
	const int32_t dx = x - width / 2;
	const int32_t dy = y - height / 2;
	return (dx * dx + dy * dy) <= gameState->values.playingFieldDeadzone * gameState->values.playingFieldDeadzone;
}

float PlayingField::distanceToSporeType(glm::vec2 pos, SporeType sporeType)
{
	float dist = std::numeric_limits<float>::max();
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			uint32_t dx = pos.x - x;
			uint32_t dy = pos.y - y;
			float currDist = sqrt(dx * dx + dy * dy);
			if ((cells[x][y].sporeType == sporeType) && ((x != pos.x) || (y != pos.y)) && (currDist < dist)) {
				dist = currDist;
			}
		}
	}
	return dist;
}

Cell* PlayingField::cellFromVisualPos(glm::vec3 pos)
{
	glm::ivec2 cellPos = { round(pos.x / gridSize) + width / 2, round(pos.z / gridSize) + height / 2 };
	if ((cellPos.x > -1) && (cellPos.x < width) && (cellPos.y > -1) && (cellPos.y < height)) {
		return &cells[cellPos.x][cellPos.y];
	}
	return nullptr;
}

Cell* PlayingField::cellAt(glm::ivec2 pos)
{
	if ((pos.x > -1) && (pos.x < width) && (pos.y > -1) && (pos.y < height)) {
		return &cells[pos.x][pos.y];
	}
	return nullptr;
}

void PlayingField::getCellsAtDistance(glm::ivec2 pos, uint32_t distance, Cell* cells[], uint32_t& count)
{
	count = 0;
	for (int32_t x = pos.x - distance; x <= pos.x + distance; x++) {
		for (int32_t y = pos.y - distance; y <= pos.y + distance; y++) {
			if ((x > -1) && (x < width) && (y > -1) && (y < height)) {
				if (std::max(abs(x - pos.x), abs(y - pos.y)) == distance) {
					if (deadZone(x, y)) {
						continue;
					}
					Cell* cell = cellAt(glm::ivec2(x, y));
					if (cell) {
						cells[count] = cell;
						count++;
					}
				}
			}
		}
	}
}

void PlayingField::prepareGPUResources()
{
	// @todo: proper sync

	// @todo: staging, ring-buffer, etc.
	const uint32_t dim = width * height;
	VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &instanceBuffer, sizeof(InstanceData) * dim));
	// Init instance data
	std::vector<InstanceData> instanceData(width * height);
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			const uint32_t idx = (x * height) + y;
			Cell& cell = cells[x][y];
			instanceData[idx].pos = glm::vec3(cell.gridPos.x + cell.rndOffset.x, 1.0f, cell.gridPos.y + cell.rndOffset.y);
			instanceData[idx].scale = 0.0f;
		}
	}

	instanceBuffer.copyTo(instanceData.data(), instanceData.size() * sizeof(InstanceData));
}

void PlayingField::updateGPUResources()
{
	bool updateBuffer = false;
	std::vector<InstanceData> instanceData(width * height);
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			const uint32_t idx = (x * height) + y;
			if (cells[x][y].sporeSize != instanceData[idx].scale) {
				Cell& cell = cells[x][y];
				instanceData[idx].pos = glm::vec3(cell.gridPos.x + cell.rndOffset.x, 1.0f - cell.zIndex, cell.gridPos.y + cell.rndOffset.y);
				instanceData[idx].scale = cell.sporeSize;
				//if (cell.sporeType == SporeType::Good_Portal) {
				//	instanceData[idx].pos.y = -2.0f;
				//}
				updateBuffer = true;
			}
		}
	}
	if (updateBuffer) {
		// @todo: Proper sync
		instanceBuffer.copyTo(instanceData.data(), instanceData.size() * sizeof(InstanceData));
	}
}

void PlayingField::draw(CommandBuffer* cb)
{
	// @todo
}

void PlayingField::save(std::ofstream& stream)
{
	stream.write((const char*)&width, sizeof(width));
	stream.write((const char*)&height, sizeof(height));
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			uint32_t sporeType = (uint32_t)cell.sporeType;
			stream.write((const char*)&sporeType, sizeof(sporeType));
			stream.write((const char*)&cell.sporeSize, sizeof(cell.sporeSize));
			stream.write((const char*)&cell.floatValue, sizeof(cell.floatValue));
		}
	}
}

void PlayingField::load(std::ifstream& stream)
{
	stream.read((char*)&width, sizeof(width));
	stream.read((char*)&height, sizeof(height));
	generate(width, height);
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			stream.read((char*)&cell.sporeType, sizeof(cell.sporeSize));
			stream.read((char*)&cell.sporeSize, sizeof(cell.sporeSize));
			stream.read((char*)&cell.floatValue, sizeof(cell.floatValue));
		}
	}
}
