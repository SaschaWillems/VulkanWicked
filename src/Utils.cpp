/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Utils.h"

int32_t randomInt(int32_t range)
{
return rand() % range;
}

float randomFloat(float range)
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX / range);
}
