/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <glm/glm.hpp>

class LightSource
{
public:
	glm::vec4 position;
	glm::vec3 color;
	float radius;
};

