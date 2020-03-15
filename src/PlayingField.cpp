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

	cells[width / 4][height / 2].sporeType = SporeType::Good_Portal;
	cells[width / 4][height / 2].sporeSize = 1.0f;
	cells[width - width / 4][height / 2].sporeType = SporeType::Evil_Portal;
	cells[width - width / 4][height / 2].sporeSize = 1.0f;
}

void PlayingField::update(float dT)
{
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			if (cell.sporeType == SporeType::Evil_Dead) {
				cell.floatValue -= dT * gameState->values.evilDeadSporeRessurectionSpeed;
				if (cell.floatValue <= 0) {
					// Dead evil cells with more good than evil full-grown neighbours turn into good cells
					// @todo: only at day?
					uint32_t neighbourCountGood = getNeighbourCount(x, y, SporeType::Good);
					uint32_t neighbourCountEvil = getNeighbourCount(x, y, SporeType::Evil);
					if (neighbourCountGood > neighbourCountEvil) {
						cell.sporeType = SporeType::Good;
					}
					else {
						// Dead evil spores return to live after a certain amount of time and if not overtaken by good growth
						cell.sporeType = SporeType::Evil;
					}
				}
			}
		}
	}

	// Growth
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			if (cell.sporeType == SporeType::Good_Portal || cell.sporeType == SporeType::Evil_Portal) {
				// Reworked growth functionality based around portals (seems to be what the original is doing)
				// If growth timer triggers, either spawn a new small spore in portals' range or grow an existing one
				// Spawning is done in growing distance around the portal, once all spores at a given distance are maxed out, it's possible for spores to spawn further away
				// @todo: Add some more randomization
				// Portal growth timer speed depends on day/night cycle
				if (cell.sporeType == SporeType::Good_Portal) {
					cell.portalGrowTimer -= dT * ((gameState->phase == Phase::Day) ? gameState->values.portalGrowthSpeedFast : gameState->values.portalGrowthSpeedSlow) * gameState->values.portalGrowthFactorGood;
				}
				if (cell.sporeType == SporeType::Evil_Portal) {
					cell.portalGrowTimer -= dT * ((gameState->phase == Phase::Night) ? gameState->values.portalGrowthSpeedFast : gameState->values.portalGrowthSpeedSlow) * gameState->values.portalGrowthFactorEvil;
				}
				if (cell.portalGrowTimer <= 0.0f) {
					// @todo: Add randomization;
					cell.portalGrowTimer = gameState->values.portalGrowTimer;
					int32_t maxDist = 3;
					int32_t currentDist = 1;
					std::vector<Cell*> cells;
					// @todo: slow down growth chance based on current distance (1 = 100%, 2 = 50%, 3 = 25%, etc.)
					while (true) {
						// Check if there is any cell at the current distance from the portal that's not fully grown
						getCellsAtDistance(glm::ivec2(x, y), currentDist, cells);
						if (cell.sporeType == SporeType::Evil_Portal) {
							cells.erase(std::remove_if(cells.begin(), cells.end(), [](const Cell* c) { return c->sporeType == SporeType::Evil_Dead; }), cells.end());
						}

						// @todo: also take portals into account
						bool skip = true;
						for (auto dstCell : cells) {
							if (dstCell && (dstCell->sporeType == SporeType::Empty || dstCell->sporeSize < SporeSize::Max)) {
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
						int growthChance = std::max(100.0f - ((currentDist - 1) * 50.0f), 10.0f);
						if (randomFloat(100.0f) > growthChance) {
							continue;
						}
						Cell* dstCell = cells[randomInt(static_cast<int32_t>(cells.size()))];
						if (dstCell->sporeType == SporeType::Empty) {
							dstCell->sporeType = (cell.sporeType == SporeType::Good_Portal) ? SporeType::Good : SporeType::Evil;
							dstCell->sporeSize = SporeSize::Small;
							dstCell->zIndex = getMaxCellZIndex(dstCell->pos);
							break;
						}
						else {
							if (dstCell->canGrow()) {
								dstCell->grow();
								// Bring forward
								dstCell->zIndex = getMaxCellZIndex(dstCell->pos);
								break;
							}
						}
					}
				}
			}
		}
	}
}

uint32_t PlayingField::getNeighbourCount(uint32_t x, uint32_t y, SporeType sporeType)
{
	if ((x < 0) || (x > width) | (y < 0) && (y > height)) {
		return 0;
	}
	uint32_t count = 0;
	const std::vector<glm::ivec2> offsets = { {0, -1}, {1, -1}, {1,  0}, {1,  1}, {0,  1}, {-1,  1}, {-1,  0}, {-1, -1} };
	for (auto offset : offsets) {
		glm::ivec2 dst = glm::vec2(x + offset.x, y + offset.y);
		if ((dst.x > -1) && (dst.x < width) && (dst.y > -1) && (dst.y < height)) {
			Cell& dstCell = cells[dst.x][dst.y];
			if (dstCell.sporeType == sporeType && dstCell.sporeSize >= 0.75f) {
				count++;
			}
		}
	}
	return count;
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

void PlayingField::getCellsAtDistance(glm::ivec2 pos, uint32_t distance, std::vector<Cell*> &cells, bool skipDeadZone)
{
	cells.clear();
	for (int32_t x = pos.x - distance; x <= pos.x + distance; x++) {
		for (int32_t y = pos.y - distance; y <= pos.y + distance; y++) {
			if ((x > -1) && (x < width) && (y > -1) && (y < height)) {
				if (std::max(abs(x - pos.x), abs(y - pos.y)) == distance) {
					if (skipDeadZone && deadZone(x, y)) {
						continue;
					}
					Cell* cell = cellAt(glm::ivec2(x, y));
					if (cell) {
						cells.push_back(cell);
					}
				}
			}
		}
	}
}

float PlayingField::getMaxCellZIndex(glm::ivec2 pos)
{
	float maxZ = cellAt(pos)->zIndex;
	for (int32_t x = pos.x - 1; x <= pos.x + 1; x++) {
		for (int32_t y = pos.y - 1; y <= pos.y + 1; y++) {
			Cell* cell = cellAt(glm::ivec2(x, y));
			if (cell && cell->zIndex > maxZ) {
				maxZ = cell->zIndex;
			}
		}
	}
	return std::min(maxZ + 0.1f, 256.0f);
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
