/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>

struct BoundingBox
{
	float left, right, top, bottom;
	BoundingBox();
	BoundingBox(float left, float right, float top, float bottom);
	float aspectRatio();
	float width();
	float height();
	bool inside(glm::vec3 pos, float margin = 0.0f);
};