/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <sys/stat.h>

#include "Device.h"
#include "Texture.h"
#include "VulkanTools.h"
#include "VulkanglTFModel.h"

class AssetManager
{
private:
	Device* device;
	VkQueue transferQueue;
	std::map<std::string, vkglTF::Model*> models;
	std::unordered_map<std::string, Texture*> textures;
public:
	std::string assetPath;
	void addModelsFolder(std::string folder);
	vkglTF::Model* getModel(std::string name);
	void addTexturesFolder(std::string folder);
	AssetManager();
	void setDevice(Device* device);
	void setTransferQueue(VkQueue queue);
	VkShaderModule loadShaderShaderModule(std::string filename);
};

extern AssetManager* assetManager;

