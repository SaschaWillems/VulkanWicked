/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
 
#pragma once

#include "VulkanRenderer.h"

class RenderObject
{
public:
	VulkanRenderer* renderer = nullptr;
	void setRenderer(VulkanRenderer* renderer); 
};

