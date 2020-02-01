/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Framebuffer.h"

Framebuffer::Framebuffer(VkDevice device) 
{
	this->device = device;
}

Framebuffer::~Framebuffer() {
    vkDestroyFramebuffer(device, handle, nullptr);
}