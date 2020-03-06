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
			cell.gridPos = glm::vec2(-(width * gridSize / 2.0f) + x * gridSize + gridSize / 2.0f, -(height * gridSize / 2.0f) + y * gridSize + gridSize / 2.0f);
			cell.rndOffset = glm::vec2(rndFloat(rndEngine), rndFloat(rndEngine));
			cell.rndOffset = glm::vec2(0.0f);
		}
	}

	cells[width / 4][height / 2].sporeType = SporeType::Good_Portal;
	cells[width - width / 4][height / 2].sporeType = SporeType::Evil_Portal;
}

void PlayingField::update(float dT)
{
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			Cell& cell = cells[x][y];
			// @todo: other Spore types, move to class
			if (cell.sporeType == SporeType::Good) {
				if (cell.sporeSize < gameState->values.maxSporeSize) {
					cell.sporeSize += ((gameState->phase == Phase::Day) ? gameState->values.growthSpeedFast : gameState->values.growthSpeedSlow) * dT;
				}
			}
			if (cell.sporeType == SporeType::Evil) {
				if (cell.sporeSize < gameState->values.maxSporeSize) {
					cell.sporeSize += ((gameState->phase == Phase::Night) ? gameState->values.growthSpeedFast : gameState->values.growthSpeedSlow) * dT;
				}
			}
			if (cell.sporeType == SporeType::Evil_Dead) {
				// Dead evil spores return to live after a certain amount of time and if not overtaken by good growth
				cell.floatValue -= dT * gameState->values.evilDeadSporeRessurectionSpeed;
				if (cell.floatValue <= 0) {
					cell.sporeType = SporeType::Evil;
				}
			}
			if (cell.sporeType == SporeType::Good_Portal || cell.sporeType == SporeType::Evil_Portal) {
				cell.sporeSize = 1.0f;
			}
		}
	}

	// Growth
	// @todo: maybe calculate from spore pov, check distance and connection to portal
	// Spawn
	const std::vector<glm::ivec2> offsets = { {0, -1}, {1, -1}, {1,  0}, {1,  1}, {0,  1}, {-1,  1}, {-1,  0}, {-1, -1} };
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			if (!deadZone(x, y)) {
				Cell& cell = cells[x][y];
				if (cell.sporeType == SporeType::Good_Portal || cell.sporeType == SporeType::Evil_Portal) {
					for (auto offset : offsets) {
						glm::ivec2 dst = glm::vec2(x + offset.x, y + offset.y);
						if ((dst.x > -1) && (dst.x < width) && (dst.y > -1) && (dst.y < height)) {
							Cell& dstCell = cells[dst.x][dst.y];
							if (dstCell.sporeType == SporeType::Empty) {
								dstCell.sporeType = (cell.sporeType == SporeType::Good_Portal) ? SporeType::Good : SporeType::Evil;
							}
						}
					}
				}
			}
		}
	}
	// Growth
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			if (!deadZone(x, y)) {
				Cell& cell = cells[x][y];
				uint32_t neighbourCountGood = getNeighbourCount(x, y, SporeType::Good);
				uint32_t neighbourCountEvil = getNeighbourCount(x, y, SporeType::Evil);
				// @todo: distance to next portal for growth factor increase
				// Empty cell grows for highest neighbour type count
				if (cell.empty() && (neighbourCountGood >= 2 || neighbourCountEvil >= 2)) {
					// Distance to next portal limits growth
					// @todo: also check if actually connected to that portal? (nextPortal instead of just distance?)
					float distanceToPortal = distanceToSporeType({ x, y }, (neighbourCountGood > neighbourCountEvil) ? SporeType::Good_Portal : SporeType::Evil_Portal);
					if (distanceToPortal < gameState->values.maxGrowthDistanceToPortal) {
						cell.sporeType = (neighbourCountGood > neighbourCountEvil) ? SporeType::Good : SporeType::Evil;
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
				instanceData[idx].pos = glm::vec3(cell.gridPos.x + cell.rndOffset.x, 1.0f, cell.gridPos.y + cell.rndOffset.y);
				instanceData[idx].scale = cell.sporeSize;
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
