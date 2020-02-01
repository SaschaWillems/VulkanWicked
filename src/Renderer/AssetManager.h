/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include <map>
#include <filesystem>
#include <sys/stat.h>

#include "Device.h"
#include "VulkanTools.h"
#include "VulkanglTFModel.h"

class AssetManager
{
private:
	Device* device;
	VkQueue transferQueue;
	std::map<std::string, vkglTF::Model*> models;
public:
	std::string assetPath;
	void addModelsFolder(std::string folder);
	vkglTF::Model* getModel(std::string name);
	AssetManager();
	void setDevice(Device* device);
	void setTransferQueue(VkQueue queue);
	VkShaderModule loadShaderShaderModule(std::string filename);
};

extern AssetManager* assetManager;

