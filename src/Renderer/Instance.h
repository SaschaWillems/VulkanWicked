/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <string>
#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanTools.h"

class Instance
{
private:
	const char* applicationName;
	uint32_t applicationVersion{ 0 };
	const char* engineName;
	uint32_t engineVersion{ 0 };
	uint32_t apiVersion{ VK_API_VERSION_1_0 };
	void* pNext = nullptr;
	std::vector<const char*> enabledExtensions;
	std::vector<const char*> enabledLayers;
	std::vector<std::string> supportedExtensions;
	std::vector<std::string> supportedLayers;
public:
	VkInstance handle = VK_NULL_HANDLE;
	Instance();
	~Instance();
	void setApplicationName(const char* name);
	void setApplicationVersion(uint32_t version);
	void setEngineName(const char* name);
	void setEngineVersion(uint32_t version);
	void setApiVersion(uint32_t version);
	void setPNext(void* pNext);
	void enableExtension(const char* extension);
	void enableLayer(const char* extension);
	void create();
};

