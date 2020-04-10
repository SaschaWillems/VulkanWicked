/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <time.h>
#include <vector>
#include <random>

#include "Renderer/RenderObject.h"
#include "Renderer/CommandBuffer.h"
#include "GameState.h"
#include "Cell.h"
#include "Utils.h"

#pragma once
class PlayingField: public RenderObject
{
private:
	// @todo: remove after restructuring?
	struct InstanceData {
		glm::vec3 pos;
		float scale;
	};
	void updatePortal(Cell* portal, float dT);
	void getCellsAtDistance(glm::ivec2 pos, uint32_t distance, Cell* cells[], uint32_t& count);
public:
	// @todo: make private after restructuring
	Buffer instanceBuffer;
	const float gridSize = 1.3f;
	uint32_t width;
	uint32_t height;
	std::vector<std::vector<Cell>> cells;
	void generate(uint32_t width, uint32_t height);
	void clear();
	void update(float dT);
	bool deadZone(uint32_t x, uint32_t y);
	float distanceToSporeType(glm::vec2 pos, SporeType sporeType);
	Cell* cellFromVisualPos(glm::vec3 pos);
	Cell* cellAt(glm::ivec2 pos);
	void prepareGPUResources();
	void updateGPUResources();
	void draw(CommandBuffer* cb);
	void save(std::ofstream& stream);
	void load(std::ifstream& stream);
};

extern PlayingField* playingField;
