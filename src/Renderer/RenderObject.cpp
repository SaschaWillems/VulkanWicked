/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "RenderObject.h"

void RenderObject::setRenderer(VulkanRenderer* renderer)
{
	this->renderer = renderer;
}
