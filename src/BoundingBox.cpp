/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "BoundingBox.h"

BoundingBox::BoundingBox()
{
}

BoundingBox::BoundingBox(float left, float right, float top, float bottom)
{
	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
}

float BoundingBox::aspectRatio()
{
	return (right - left) / (bottom - top);
}

float BoundingBox::width()
{
	return (right - left);
}

float BoundingBox::height()
{
	return (bottom - top);
}

bool BoundingBox::inside(glm::vec3 pos, float margin)
{
	return (pos.x >= left - abs(left * margin)) && (pos.x <= right + abs(right * margin)) && (pos.z >= top - abs(top * margin)) && (pos.z <= bottom + abs(bottom * margin));
}
