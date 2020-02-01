/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Image.h"

Image::Image(Device* device) {
	this->device = device;
}

Image::~Image() {
	vkFreeMemory(device->handle, memory, nullptr);
	vkDestroyImage(device->handle, handle, nullptr);
}

void Image::create() {
	VkImageCreateInfo CI = vks::initializers::imageCreateInfo();
	CI.imageType = type;
	CI.format = format;
	CI.extent = extent;
	CI.mipLevels = mipLevels;
	CI.arrayLayers = arrayLayers;
	CI.samples = samples;
	CI.tiling = tiling;
	CI.usage = usage;
	VK_CHECK_RESULT(vkCreateImage(device->handle, &CI, nullptr, &handle));
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device->handle, handle, &memReqs);
	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device->handle, &memAlloc, nullptr, &memory));
	VK_CHECK_RESULT(vkBindImageMemory(device->handle, handle, memory, 0));
}

void Image::setType(VkImageType type) {
	this->type = type;
}

void Image::setFormat(VkFormat format) {
	this->format = format;
}

void Image::setExtent(VkExtent3D extent) {
	this->extent = extent;
}

void Image::setNumMipLevels(uint32_t mipLevels) {
	this->mipLevels = mipLevels;
}

void Image::setNumArrayLayers(uint32_t arrayLayers) {
	this->arrayLayers = arrayLayers;
}

void Image::setSampleCount(VkSampleCountFlagBits samples) {
	this->samples = samples;
}
void Image::setTiling(VkImageTiling tiling) {
	this->tiling = tiling;
}

void Image::setUsage(VkImageUsageFlags usage) {
	this->usage = usage;
}

void Image::setSharingMode(VkSharingMode sharingMode) {
	this->sharingMode = sharingMode;
}