/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"
#include "Device.h"
#include "Image.h"

class ImageView {
private:
	Device* device = nullptr;
	Image* image = nullptr;
	VkImageViewType type;
	VkFormat format;
	VkImageSubresourceRange range;
public:
	VkImageView handle;
	ImageView(Device* device);
	~ImageView();
	void create();
	void setImage(Image* image);
	void setType(VkImageViewType type);
	void setFormat(VkFormat format);
	void setSubResourceRange(VkImageSubresourceRange range);
};