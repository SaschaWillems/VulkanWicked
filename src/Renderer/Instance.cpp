/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Instance.h"

Instance::Instance()
{
}

Instance::~Instance()
{
	vkDestroyInstance(handle, nullptr);
}

void Instance::setApplicationName(const char* name)
{
	applicationName = name;
}

void Instance::setApplicationVersion(uint32_t version)
{
	applicationVersion = version;
}

void Instance::setEngineName(const char* name)
{
	engineName = name;
}

void Instance::setEngineVersion(uint32_t version)
{
	engineVersion = version;
}

void Instance::setApiVersion(uint32_t version)
{
	apiVersion = version;
}

void Instance::setPNext(void* pNext)
{
	this->pNext = pNext;
}

void Instance::enableExtension(const char* extension)
{
	enabledExtensions.push_back(extension);
}

void Instance::enableLayer(const char* extension)
{
	enabledLayers.push_back(extension);
}

void Instance::create()
{
	VkApplicationInfo AI{};
	AI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AI.pEngineName = engineName;
	AI.engineVersion = engineVersion;
	AI.pApplicationName = applicationName;
	AI.applicationVersion = applicationVersion;
	AI.apiVersion = apiVersion;
	VkInstanceCreateInfo CI{};
	CI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CI.pApplicationInfo = &AI;
	CI.pNext = pNext;
	CI.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	CI.ppEnabledExtensionNames = enabledExtensions.data();
	CI.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
	CI.ppEnabledLayerNames = enabledLayers.data();
	VK_CHECK_RESULT(vkCreateInstance(&CI, nullptr, &handle));
}
