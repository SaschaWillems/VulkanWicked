/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ImageView.h"

ImageView::ImageView(Device* device) 
{
	this->device = device;
}

ImageView::~ImageView() 
{
	vkDestroyImageView(device->handle, handle, nullptr);
}

void ImageView::create() 
{
	VkImageViewCreateInfo CI = vks::initializers::imageViewCreateInfo();
	CI.viewType = type;
	CI.format = format;
	CI.subresourceRange = range;
	CI.image = image->handle;
	VK_CHECK_RESULT(vkCreateImageView(device->handle, &CI, nullptr, &handle));
}

void ImageView::setImage(Image* image) 
{
	this->image = image;
}

void ImageView::setType(VkImageViewType type)
{
	this->type = type;
}

void ImageView::setFormat(VkFormat format)
{
	this->format = format;
}

void ImageView::setSubResourceRange(VkImageSubresourceRange range)
{
	this->range = range;
}