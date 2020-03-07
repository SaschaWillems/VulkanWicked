/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "AssetManager.h"

AssetManager* assetManager = nullptr;

void AssetManager::addModelsFolder(std::string folder)
{
	for (const auto& file : std::filesystem::directory_iterator(assetPath + folder)) {
		const std::string name = file.path().stem().string();
		const std::string ext = file.path().extension().string();
		if (ext != ".gltf") {
			continue;
		}
		vkglTF::Model* model = new vkglTF::Model();
		model->loadFromFile(file.path().string(), device, transferQueue);
		models[name] = model;
		std::clog << "Model \"" << name << "\" added from \"" << folder << "\"" << std::endl;
	}
}

vkglTF::Model* AssetManager::getModel(std::string name)
{
	return models[name];
}

void AssetManager::addTexturesFolder(std::string folder)
{
	for (const auto& file : std::filesystem::directory_iterator(assetPath + folder)) {
		const std::string name = file.path().stem().string();
		Texture2D* texture = new Texture2D();
		texture->loadFromFile(file.path().string(), device, transferQueue);
		textures[name] = texture;
		std::clog << "Texture \"" << name << "\" added from \"" << folder << "\"" << std::endl;
	}
}

Texture* AssetManager::getTexture(std::string name)
{
	return textures[name];
}

AssetManager::AssetManager()
{
	std::vector<std::string> paths = { "./data", "./../data/" };
	for (auto path : paths) {
		struct stat info;
		if (stat(path.c_str(), &info) == 0) {
			assetPath = path;
			std::cout << "Asset path found in \"" << path << "\"" << std::endl;
			break;
		}
	}
}

void AssetManager::setDevice(Device* device)
{
	this->device = device;
}

void AssetManager::setTransferQueue(VkQueue queue)
{
	this->transferQueue = queue;
}

VkShaderModule AssetManager::loadShaderShaderModule(std::string filename)
{
	std::ifstream is(assetPath + filename, std::ios::binary | std::ios::in | std::ios::ate);
	if (is.is_open())
	{
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();
		assert(size > 0);
		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		VK_CHECK_RESULT(vkCreateShaderModule(device->handle, &moduleCreateInfo, nullptr, &shaderModule));
		delete[] shaderCode;
		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
		return VK_NULL_HANDLE;
	}
}



