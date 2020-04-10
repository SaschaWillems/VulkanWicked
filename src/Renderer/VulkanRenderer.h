/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "VulkanAndroid.h"
#endif

#include <SDL.h>
#include <SDL_Vulkan.h>

#include <iostream>
#include <chrono>
#include <unordered_map>
#include <sys/stat.h>
#include "json.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <string>
#include <array>
#include <numeric>

#include "vulkan/vulkan.h"

#include "VulkanTools.h"
#include "VulkanDebug.h"

#include "VulkanInitializers.h"
#include "Camera.h"

#include "Swapchain.h"
#include "Instance.h"
#include "Image.h"
#include "ImageView.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DescriptorPool.h"

#include "LightSource.h"

// @todo: lower limit
const uint32_t MAX_NUM_LIGHTS = 512;

enum class FramebufferType { Color, DepthStencil };

struct FrameBufferAttachment {
	VkFramebuffer frameBuffer;
	ImageView* view;
	Image* image;
	VkDescriptorImageInfo descriptor;
};

class VulkanRenderer
{
private:	
	std::unordered_map<std::string, Pipeline*> pipelines;
	std::unordered_map<std::string, PipelineLayout*> pipelineLayouts;
	std::unordered_map<std::string, RenderPass*> renderPasses;
	std::map<std::string, DescriptorSetLayout*> descriptorSetLayouts;

	Instance* instance;

	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	void windowResize();

	void createCommandPool();
	void createPipelineCache();
	void createFrameBufferImage(FrameBufferAttachment& target, FramebufferType type, VkFormat fmt = VK_FORMAT_UNDEFINED);
	void setupRenderPass();

	void setupOffscreenRenderPass();
	void setupDepthStencil();
	void setupFrameBuffer();
	void setupLayouts();
	void setupDescriptorPool();

	Pipeline* loadPipelineFromFile(std::string filename);
	void loadPipelines();
protected:
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VkFormat depthFormat;
	CommandPool* commandPool;
	std::vector<CommandBuffer*> commandBuffers;
	Swapchain* swapchain;
	struct {
		VkSemaphore presentComplete;
		VkSemaphore renderComplete;
	} semaphores;
public: 
	uint32_t width = 1280;
	uint32_t height = 720;

	uint32_t renderWidth;
	uint32_t renderHeight;

	VkPipelineCache pipelineCache;
	DescriptorPool* descriptorPool;
	Device* device;

	std::vector<VkFramebuffer>frameBuffers;
	uint32_t currentBuffer = 0;
	VkQueue queue;

	struct Settings {
		bool validation = false;
		bool console = false;
		bool fullscreen = false;
		bool vsync = false;
		bool debugoverlay = false;
		bool crtshader = false;
	} settings;

	static std::vector<const char*> args;

	Camera camera;

	SDL_Window* window;

	// Vulkan resources

	struct DescriptorSets {
		DescriptorSet* debugquad;
		DescriptorSet* camera;
	} descriptorSets;

	// @todo: Multiple frames in flight
	CommandBuffer* commandBuffer;
	VkFence cbWaitFence;

	// Attachments
	Image* depthStencilImage;
	ImageView* depthStencilImageView;

	// Offscreen rendering pass resources (MRT)

	struct OffscreenPass {
		int32_t width, height;
		FrameBufferAttachment position, normal, albedo, depth, material, pbr;
		VkFramebuffer frameBuffer;
		VkSampler sampler;
	} offscreenPass;

	// Deferred composition pass resources
	
	struct DeferredComposition {
		DescriptorSet* descriptorSet;
		Buffer lightsBuffer;
	} deferredComposition;

	// Buffers 

	// @todo: Remove after restructuring
	struct InstanceData {
		glm::vec3 pos;
		float scale;
	};

	struct UBO {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightDir = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
	} uboShared;

	// @todo: rename
	struct LightSources {
		LightSource lights[MAX_NUM_LIGHTS];
		glm::vec4 viewPos;
		glm::vec2 screenRes;
		glm::vec2 renderRes;
		int32_t numLights;
		float fade = 0.5f;
		float desaturate = 0.0f;
		int32_t scanlines = 0;
	} lightSources;
	Buffer lightsSSBO;

	VulkanRenderer();
	virtual ~VulkanRenderer();

	VkResult createInstance(bool enableValidation);

	void waitSync();
	void submitFrame();
	
	void addLight(LightSource lightSource);

	PipelineLayout* addPipelineLayout(std::string name);
	PipelineLayout* getPipelineLayout(std::string name);
	Pipeline* addPipeline(std::string name);
	Pipeline* getPipeline(std::string name);
	RenderPass* addRenderPass(std::string name);
	RenderPass* getRenderPass(std::string name);
	DescriptorSetLayout* addDescriptorSetLayout(std::string name);
	DescriptorSetLayout* getDescriptorSetLayout(std::string name);
};