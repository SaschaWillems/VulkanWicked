/* Copyright (c) 2020-2023, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "VulkanRenderer.h"

std::vector<const char*> VulkanRenderer::args;

VkResult VulkanRenderer::createInstance(bool enableValidation)
{
	this->settings.validation = enableValidation;

	instance = new Instance();
	instance->setApiVersion(VK_API_VERSION_1_0);
	instance->setApplicationName("VulkanWicked");
	instance->setEngineName("VulkanWicked");

	// Enable surface extensions depending on os
	unsigned int extCount{ 0 };
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, NULL)) {
		std::cout << "Could not get instance extensions from SDL." << std::endl;
	}
	std::vector<const char*> instanceExtensions(extCount);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, instanceExtensions.data())) {
		std::cout << "Could not get instance extensions from SDL." << std::endl;
	}
	for (auto extension: instanceExtensions)
	{
		instance->enableExtension(extension);
	}
	instance->enableExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	if (settings.validation)
	{
		instance->enableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			instance->enableLayer(validationLayerName);
		} else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}
	instance->create();
	return VK_SUCCESS;
}

void VulkanRenderer::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(device->handle, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void VulkanRenderer::waitSync()
{
	VK_CHECK_RESULT(vkWaitForFences(device->handle, 1, &cbWaitFence, VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(device->handle, 1, &cbWaitFence));
}

void VulkanRenderer::submitFrame()
{
	// Acquire the next image from the swap chain
	VkResult result = swapchain->acquireNextImage(semaphores.presentComplete, &currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		windowResize();
	}
	else {
		VK_CHECK_RESULT(result);
	}

	const VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = vks::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer->handle;
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, cbWaitFence));

	result = swapchain->queuePresent(queue, currentBuffer, semaphores.renderComplete);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// Swap chain is no longer compatible with the surface and needs to be recreated
			windowResize();
			return;
		} else {
			VK_CHECK_RESULT(result);
		}
	}
}

VulkanRenderer::VulkanRenderer()
{
	char* numConvPtr;
	// Parse command line arguments
	for (size_t i = 0; i < args.size(); i++)
	{
		if (args[i] == std::string("-validation")) {
			settings.validation = true;
		}
		if ((args[i] == std::string("-c")) || (args[i] == std::string("--console"))) {
			settings.console = true;
		}
		if ((args[i] == std::string("-d")) || (args[i] == std::string("--debugoverlay"))) {
			settings.debugoverlay = true;
		}
		if (args[i] == std::string("-vsync")) {
			settings.vsync = true;
		}
		if ((args[i] == std::string("-f")) || (args[i] == std::string("--fullscreen"))) {
			settings.fullscreen = true;
		}
		if ((args[i] == std::string("-w")) || (args[i] == std::string("-width"))) {
			uint32_t w = strtol(args[i + 1], &numConvPtr, 10);
			if (numConvPtr != args[i + 1]) { width = w; };
		}
		if ((args[i] == std::string("-h")) || (args[i] == std::string("-height"))) {
			uint32_t h = strtol(args[i + 1], &numConvPtr, 10);
			if (numConvPtr != args[i + 1]) { height = h; };
		}
		if (args[i] == std::string("--crt")) {
			settings.crtshader = true;
		}
	}

	renderWidth = settings.crtshader ? 320.0f * 2.0f : width;
	renderHeight = settings.crtshader ? 200.0f * 2.0f : height;

#if defined(_WIN32)
	if (this->settings.validation || this->settings.console)
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w+", stdout);
		freopen_s(&stream, "CONOUT$", "w+", stderr);
	}
#endif
	window = SDL_CreateWindow("Vulkan Wicked", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
	if (settings.fullscreen) {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Vulkan library is loaded dynamically on Android
	bool libLoaded = vks::android::loadVulkanLibrary();
	assert(libLoaded);
#endif

	// Vulkan initialization
	VkResult err;

	err = createInstance(settings.validation);
	if (err) {
		vks::tools::exitFatal("Could not create Vulkan instance : \n" + vks::tools::errorString(err), err);
	}

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	vks::android::loadVulkanFunctions(instance);
#endif

	if (settings.validation)
	{
		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		vks::debug::setupDebugging(instance->handle, debugReportFlags, VK_NULL_HANDLE);
	}

	uint32_t gpuCount = 0;
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance->handle, &gpuCount, nullptr));
	assert(gpuCount > 0);
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(instance->handle, &gpuCount, physicalDevices.data());
	if (err) {
		vks::tools::exitFatal("Could not enumerate physical devices : \n" + vks::tools::errorString(err), err);
	}

	physicalDevice = physicalDevices[0];

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	device = new Device(physicalDevice, instance);
	device->enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
	device->enabledFeatures.independentBlend = VK_TRUE;
	VkResult res = device->create();
	if (res != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(res), res);
	}

	vkGetDeviceQueue(device->handle, device->queueFamilyIndices.graphics, 0, &queue);

	assetManager->setDevice(device);
	assetManager->setTransferQueue(queue);

	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	assert(validDepthFormat);

	VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
	VK_CHECK_RESULT(vkCreateSemaphore(device->handle, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
	VK_CHECK_RESULT(vkCreateSemaphore(device->handle, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));

	swapchain = new Swapchain(instance->handle, device->handle, device->physicalDevice);
	swapchain->initSurface(window);
	swapchain->create(&width, &height, settings.vsync);

	createCommandPool();
	setupDepthStencil();
	setupRenderPass();
	setupOffscreenRenderPass();
	createPipelineCache();
	setupFrameBuffer();

	VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VK_CHECK_RESULT(vkCreateFence(device->handle, &fenceCreateInfo, nullptr, &cbWaitFence));
	commandBuffer = new CommandBuffer(device->handle);
	commandBuffer->setPool(commandPool);
	commandBuffer->create();

	setupLayouts();
	loadPipelines();
	setupDescriptorPool();

	// Deferred composition
	VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &deferredComposition.lightsBuffer, sizeof(deferredUniformData)));
	deferredUniformData.screenRes = glm::vec2(width, height);
	deferredUniformData.renderRes = glm::vec2(renderWidth, renderHeight);
	deferredUniformData.scanlines = settings.crtshader;

	deferredComposition.descriptorSet = new DescriptorSet(device->handle);
	deferredComposition.descriptorSet->setPool(descriptorPool);
	deferredComposition.descriptorSet->addLayout(getDescriptorSetLayout("deferred_composition"));
	deferredComposition.descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.position.descriptor);
	deferredComposition.descriptorSet->addDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.normal.descriptor);
	deferredComposition.descriptorSet->addDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.albedo.descriptor);
	deferredComposition.descriptorSet->addDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.material.descriptor);
	deferredComposition.descriptorSet->addDescriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.pbr.descriptor);
	deferredComposition.descriptorSet->addDescriptor(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &deferredComposition.lightsBuffer.descriptor);
	deferredComposition.descriptorSet->create();

	// Camera
	camera.prepareGPUResources(device);
	descriptorSets.camera = new DescriptorSet(device->handle);
	descriptorSets.camera->setPool(descriptorPool);
	descriptorSets.camera->addLayout(getDescriptorSetLayout("single_ubo"));
	descriptorSets.camera->addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &camera.ubo.descriptor);
	descriptorSets.camera->create();
}

VulkanRenderer::~VulkanRenderer()
{
	SDL_Quit();

	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(device->handle, frameBuffers[i], nullptr);
	}

	delete swapchain;
	delete depthStencilImage;
	delete depthStencilImageView;

	vkDestroyPipelineCache(device->handle, pipelineCache, nullptr);

	vkDestroySemaphore(device->handle, semaphores.presentComplete, nullptr);
	vkDestroySemaphore(device->handle, semaphores.renderComplete, nullptr);
	vkDestroyFence(device->handle, cbWaitFence, nullptr);

	vkDestroySampler(device->handle, offscreenPass.sampler, nullptr);

	if (settings.validation)
	{
		vks::debug::freeDebugCallback(instance->handle);
	}
	
	delete instance;
}

void VulkanRenderer::createCommandPool()
{
	commandPool = new CommandPool(device->handle);
	commandPool->setFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool->setQueueFamilyIndex(swapchain->queueNodeIndex);
	commandPool->create();
}

void VulkanRenderer::setupDepthStencil()
{
	depthStencilImage = new Image(device);
	depthStencilImage->setType(VK_IMAGE_TYPE_2D);
	depthStencilImage->setFormat(depthFormat);
	depthStencilImage->setExtent({ width, height, 1 });
	depthStencilImage->setTiling(VK_IMAGE_TILING_OPTIMAL);
	depthStencilImage->setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthStencilImage->create();

	depthStencilImageView = new ImageView(device);
	depthStencilImageView->setType(VK_IMAGE_VIEW_TYPE_2D);
	depthStencilImageView->setFormat(depthFormat);
	if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		depthStencilImageView->setSubResourceRange({ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });
	}
	else {
		depthStencilImageView->setSubResourceRange({ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
	}
	depthStencilImageView->setImage(depthStencilImage);
	depthStencilImageView->create();
}

void VulkanRenderer::setupFrameBuffer()
{
	VkImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depthStencilImageView->handle;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = getRenderPass("deferred_composition")->handle;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	frameBuffers.resize(swapchain->imageCount);
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = swapchain->buffers[i].view;
		VK_CHECK_RESULT(vkCreateFramebuffer(device->handle, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

void VulkanRenderer::createFrameBufferImage(FrameBufferAttachment& target, FramebufferType type, VkFormat fmt, const char* name)
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspectMask;
	VkImageUsageFlags usageFlags;
	switch (type) {
	case FramebufferType::Color:
		format = (fmt == VK_FORMAT_UNDEFINED ? swapchain->colorFormat : fmt);
		usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case FramebufferType::DepthStencil:
		format = fmt;
		usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	}
	assert(format != VK_FORMAT_UNDEFINED);

	target.image = new Image(device);
	target.image->setType(VK_IMAGE_TYPE_2D);
	target.image->setFormat(format);
	target.image->setExtent({ (uint32_t)offscreenPass.width, (uint32_t)offscreenPass.height, 1 });
	target.image->setTiling(VK_IMAGE_TILING_OPTIMAL);
	target.image->setUsage(usageFlags);
	target.image->create();

	target.view = new ImageView(device);
	target.view->setType(VK_IMAGE_VIEW_TYPE_2D);
	target.view->setFormat(format);
	target.view->setSubResourceRange({ aspectMask, 0, 1, 0, 1 });
	target.view->setImage(target.image);
	target.view->create();

	target.descriptor = { offscreenPass.sampler, target.view->handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	if ((name != "") && (vks::debug::debugUtilsAvailable)) {
		device->setDebugObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)target.image->handle, name);
		device->setDebugObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)target.view->handle, name);
	}
}

void VulkanRenderer::setupRenderPass()
{
	const VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	const VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	RenderPass* renderPass = addRenderPass("deferred_composition");
	renderPass->setDimensions(width, height);
	renderPass->addSubpassDescription({
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		1,
		&colorReference,
		nullptr,
		&depthReference,
		0,
		nullptr
	});
	// Color attachment
	renderPass->addAttachmentDescription({
		0,
		swapchain->colorFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	});
	// Depth attachment
	renderPass->addAttachmentDescription({
		0,
		depthFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	});
	// Subpass dependencies
	renderPass->addSubpassDependency({
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT,
	});
	renderPass->addSubpassDependency({
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT,
	});
	renderPass->setColorClearValue(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setDepthStencilClearValue(1, 1.0f, 0);
	renderPass->create();
}

void VulkanRenderer::setupOffscreenRenderPass()
{
	// Find a suitable depth format
	VkFormat fbDepthFormat;
	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
	assert(validDepthFormat);

	std::vector<VkAttachmentReference> attachmentReferences = {
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};

	RenderPass* renderPass = addRenderPass("offscreen");
	renderPass->setDimensions(renderWidth, renderHeight);

	renderPass->addSubpassDescription({
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		static_cast<uint32_t>(attachmentReferences.size() - 1),
		attachmentReferences.data(),
		nullptr,
		& attachmentReferences[attachmentReferences.size() - 1],
		0,
		nullptr
		});

	// Color attachments
	renderPass->addAttachmentDescription({
		0,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});
	renderPass->addAttachmentDescription({
		0,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});
	renderPass->addAttachmentDescription({
		0,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});
	renderPass->addAttachmentDescription({
		0,
		VK_FORMAT_R8_UINT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});
	renderPass->addAttachmentDescription({
		0,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});
	// Depth attachment
	renderPass->addAttachmentDescription({
		0,
		fbDepthFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		});

	// Subpass dependencies
	renderPass->addSubpassDependency({
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT,
		});
	renderPass->addSubpassDependency({
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT,
		});

	renderPass->setColorClearValue(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setColorClearValue(1, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setColorClearValue(2, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setColorClearValue(3, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setColorClearValue(4, { 0.0f, 0.0f, 0.0f, 0.0f });
	renderPass->setDepthStencilClearValue(5, 1.0f, 0);
	renderPass->create();

	offscreenPass.width = renderWidth;
	offscreenPass.height = renderHeight;

	/* Renderpass */

	/* Shared sampler */

	VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device->handle, &samplerInfo, nullptr, &offscreenPass.sampler));

	/* Framebuffer images */
	createFrameBufferImage(offscreenPass.position, FramebufferType::Color, VK_FORMAT_R16G16B16A16_SFLOAT, "G-Buffer positions");
	createFrameBufferImage(offscreenPass.normal, FramebufferType::Color, VK_FORMAT_R16G16B16A16_SFLOAT, "G-Buffer normals");
	createFrameBufferImage(offscreenPass.albedo, FramebufferType::Color, VK_FORMAT_R8G8B8A8_UNORM, "G-Buffer albedo");
	createFrameBufferImage(offscreenPass.material, FramebufferType::Color, VK_FORMAT_R8_UINT, "G-Buffer materials");
	createFrameBufferImage(offscreenPass.pbr, FramebufferType::Color, VK_FORMAT_R8G8B8A8_UNORM, "G-Buffer pbr values");
	createFrameBufferImage(offscreenPass.depth, FramebufferType::DepthStencil, fbDepthFormat, "G-Buffer depth");

	/* Framebuffers */

	std::vector<VkImageView> attachments = {
		offscreenPass.position.view->handle,
		offscreenPass.normal.view->handle,
		offscreenPass.albedo.view->handle,
		offscreenPass.material.view->handle,
		offscreenPass.pbr.view->handle,
		offscreenPass.depth.view->handle
	};

	VkFramebufferCreateInfo frameBufferCI = vks::initializers::framebufferCreateInfo();
	frameBufferCI.renderPass = renderPass->handle;
	frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCI.pAttachments = attachments.data();
	frameBufferCI.width = offscreenPass.width;
	frameBufferCI.height = offscreenPass.height;
	frameBufferCI.layers = 1;
	VK_CHECK_RESULT(vkCreateFramebuffer(device->handle, &frameBufferCI, nullptr, &offscreenPass.frameBuffer));
}

void VulkanRenderer::setupLayouts()
{
	// Single UBO binding point
	DescriptorSetLayout* descriptorSetLayout;
	descriptorSetLayout = addDescriptorSetLayout("single_ubo");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	descriptorSetLayout->create();

	// Single image binding point
	descriptorSetLayout = addDescriptorSetLayout("single_image");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	// glTF PBR texture bindings
	descriptorSetLayout = addDescriptorSetLayout("gltf_pbr_images");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	PipelineLayout* pipelineLayout;
	pipelineLayout = addPipelineLayout("split_ubo");
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addPushConstantRange(sizeof(vkglTF::PushConstBlockMaterial), 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineLayout->create();

	pipelineLayout = addPipelineLayout("split_ubo_single_image");
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("single_image"));
	pipelineLayout->addPushConstantRange(sizeof(vkglTF::PushConstBlockMaterial), 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineLayout->create();

	// glTF PBR rendering (one ubo for camera, one for model, and one for pbr texture bindings)
	pipelineLayout = addPipelineLayout("gltf_pbr");
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("gltf_pbr_images"));
	pipelineLayout->addPushConstantRange(sizeof(vkglTF::PushConstBlockMaterial), 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineLayout->create();

	// Game UI
	// Single image binding point
	descriptorSetLayout = addDescriptorSetLayout("ui_text");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	pipelineLayout = addPipelineLayout("ui_text");
	pipelineLayout->addLayout(getDescriptorSetLayout("ui_text"));
	pipelineLayout->addPushConstantRange(sizeof(glm::vec2) * 2, 0, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineLayout->create();

	// Deferred composition
	descriptorSetLayout = addDescriptorSetLayout("deferred_composition");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	pipelineLayout = addPipelineLayout("deferred_composition");
	pipelineLayout->addLayout(getDescriptorSetLayout("deferred_composition"));
	pipelineLayout->create();
}

void VulkanRenderer::loadPipelines()
{
	for (const auto& file : std::filesystem::directory_iterator(assetManager->assetPath + "pipelines")) {
		const std::string ext = file.path().extension().string();
		if (ext == ".json") {
			loadPipelineFromFile(file.path().string());
		}
	}
}

void VulkanRenderer::setupDescriptorPool()
{
	// @todo: proper sizes
	descriptorPool = new DescriptorPool(device->handle);
	descriptorPool->setMaxSets(1024);
	descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024);
	descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024);
	descriptorPool->create();
}

void VulkanRenderer::windowResize()
{
	vkDeviceWaitIdle(device->handle);
	width = destWidth;
	height = destHeight;
	swapchain->create(&width, &height, settings.vsync);
	delete depthStencilImage;
	delete depthStencilImageView;
	setupDepthStencil();	
	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		vkDestroyFramebuffer(device->handle, frameBuffers[i], nullptr);
	}
	setupFrameBuffer();
	vkDeviceWaitIdle(device->handle);
	if ((width > 0.0f) && (height > 0.0f)) {
		camera.updateAspectRatio((float)width / (float)height);
	}
}

void VulkanRenderer::addLight(LightSource lightSource) {
	deferredUniformData.lights[deferredUniformData.numLights] = lightSource;
	deferredUniformData.numLights++;
}

PipelineLayout* VulkanRenderer::addPipelineLayout(std::string name)
{
	PipelineLayout* pipelineLayout = new PipelineLayout(device->handle);
	pipelineLayouts[name] = pipelineLayout;
	return pipelineLayout;
}

PipelineLayout* VulkanRenderer::getPipelineLayout(std::string name)
{
	assert(pipelineLayouts.count(name) > 0);
	return pipelineLayouts[name];
}

Pipeline* VulkanRenderer::addPipeline(std::string name)
{
	Pipeline* pipeline = new Pipeline(device->handle);
	pipelines[name] = pipeline;
	return pipeline;
}

VkBlendOp blendOpEnum(const std::string &value)
{
	if (value == "VK_BLEND_OP_ADD") {
		return VK_BLEND_OP_ADD;
	}
}

VkBlendFactor blendFactorEnum(const std::string &value)
{
	if (value == "VK_BLEND_FACTOR_ZERO") {
		return VK_BLEND_FACTOR_ZERO;
	}
	if (value == "VK_BLEND_FACTOR_ONE") {
		return VK_BLEND_FACTOR_ONE;
	}
	if (value == "VK_BLEND_FACTOR_SRC_ALPHA") {
		return VK_BLEND_FACTOR_SRC_ALPHA;
	}
	if (value == "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA") {
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}
}

VkCompareOp compareOpEnum(const std::string &value)
{
	if (value == "VK_COMPARE_OP_NEVER") {
		return VK_COMPARE_OP_NEVER;
	}
	if (value == "VK_COMPARE_OP_LESS_OR_EQUAL") {
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	}
}

VkVertexInputRate vertexInputRateEnum(const std::string &value)
{
	if (value == "VK_VERTEX_INPUT_RATE_VERTEX") {
		return VK_VERTEX_INPUT_RATE_VERTEX;
	}
	if (value == "VK_VERTEX_INPUT_RATE_INSTANCE") {
		return VK_VERTEX_INPUT_RATE_INSTANCE;
	}
}

VkFormat formatEnum(const std::string& value)
{
	if (value == "VK_FORMAT_R32_SFLOAT") {
		return VK_FORMAT_R32_SFLOAT;
	}
	if (value == "VK_FORMAT_R32G32_SFLOAT") {
		return VK_FORMAT_R32G32_SFLOAT;
	}
	if (value == "VK_FORMAT_R32G32B32_SFLOAT") {
		return VK_FORMAT_R32G32B32_SFLOAT;
	}
	if (value == "VK_FORMAT_R32G32B32A32_SFLOAT") {
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
}

Pipeline* VulkanRenderer::loadPipelineFromFile(std::string filename)
{
	std::clog << "Loading pipeline from \"" << filename << "\"" << std::endl;
	std::ifstream is(filename);
	if (!is.is_open())
	{
		std::cerr << "Error: Could not open pipeline definition file \"" << filename << "\"" << std::endl;
		return nullptr;
	}
	nlohmann::json json;
	is >> json;
	is.close();
	Pipeline* pipeline = addPipeline(json["name"]);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout(json["layout"]));
	pipeline->setRenderPass(getRenderPass(json["renderpass"]));
	for (auto& shader : json["shaders"]) {
		std::string shaderName = shader;
		pipeline->addShader("shaders/" + shaderName);
	}
	// Pipeline creation info members can be set explicitly
	// If not present, default values are applied
	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	const VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
	const VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	const VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	const VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlendStateCI;
	std::vector<VkVertexInputBindingDescription> vertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	if (json.count("depthStencilState") > 0) {
		depthStencilStateCI = {};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.front.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.depthTestEnable = json["depthStencilState"]["depthTest"];
		depthStencilStateCI.depthWriteEnable = json["depthStencilState"]["depthWrite"];
		depthStencilStateCI.depthCompareOp = compareOpEnum(json["depthStencilState"]["compareOp"]);
	}
	else {
		depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	}

	if (json.count("colorBlendState") > 0) {
		if (json["colorBlendState"].count("blendAttachments") > 0) {
			for (auto& attachment : json["colorBlendState"]["blendAttachments"]) {
				VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
				pipelineColorBlendAttachmentState.colorWriteMask = attachment["colorWriteMask"];
				pipelineColorBlendAttachmentState.blendEnable = attachment["blendEnable"];
				if (pipelineColorBlendAttachmentState.blendEnable) {
					pipelineColorBlendAttachmentState.srcColorBlendFactor = blendFactorEnum(attachment["color"]["srcFactor"]);
					pipelineColorBlendAttachmentState.dstColorBlendFactor = blendFactorEnum(attachment["color"]["dstFactor"]);
					pipelineColorBlendAttachmentState.colorBlendOp = blendOpEnum(attachment["color"]["op"]);
					pipelineColorBlendAttachmentState.srcAlphaBlendFactor = blendFactorEnum(attachment["alpha"]["srcFactor"]);
					pipelineColorBlendAttachmentState.dstAlphaBlendFactor = blendFactorEnum(attachment["alpha"]["dstFactor"]);
					pipelineColorBlendAttachmentState.alphaBlendOp = blendOpEnum(attachment["alpha"]["op"]);
				}
				blendAttachmentStates.push_back(pipelineColorBlendAttachmentState);
			}
		}
	}
	else {
		// Default setup is based on G-Buffer passes
		blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};
	}
	colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data());

	if (json.count("vertexInputState") > 0) {
		if (json["vertexInputState"].count("vertexBindingDescriptions") > 0) {
			for (auto& description : json["vertexInputState"]["vertexBindingDescriptions"]) {
				VkVertexInputBindingDescription bindingDescription{};
				bindingDescription.binding = description["binding"];
				bindingDescription.stride = description["stride"];
				bindingDescription.inputRate = vertexInputRateEnum(description["inputRate"]);
				vertexInputBindings.push_back(bindingDescription);
			}
		}
		if (json["vertexInputState"].count("vertexAttributeDescriptions") > 0) {
			for (auto& description : json["vertexInputState"]["vertexAttributeDescriptions"]) {
				VkVertexInputAttributeDescription attributeDescription{};
				attributeDescription.binding = description["binding"];
				attributeDescription.location = description["location"];
				attributeDescription.offset = description["offset"];
				attributeDescription.format = formatEnum(description["format"]);
				vertexInputAttributes.push_back(attributeDescription);
			}
		}
	}
	else {
		// Default setup is based on glTF model vertex input
		vertexInputBindings = {
			vks::initializers::vertexInputBindingDescription(0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		vertexInputAttributes = {
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos)),
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal)),
			vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv0)),
		};
	}
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	pipelineCI.pVertexInputState = &vertexInputState;
	pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;

	pipeline->setCreateInfo(pipelineCI);
	pipeline->create();

	return pipeline;
}

Pipeline* VulkanRenderer::getPipeline(std::string name)
{
	assert(pipelines.count(name) > 0);
	return pipelines[name];
}

RenderPass* VulkanRenderer::addRenderPass(std::string name)
{
	RenderPass* renderPass = new RenderPass(device->handle);
	renderPasses[name] = renderPass;
	return renderPass;
}

RenderPass* VulkanRenderer::getRenderPass(std::string name)
{
	assert(renderPasses.count(name) > 0);
	return renderPasses[name];
}

DescriptorSetLayout* VulkanRenderer::addDescriptorSetLayout(std::string name)
{
	DescriptorSetLayout* descriptorSetLayout = new DescriptorSetLayout(device->handle);
	descriptorSetLayouts[name] = descriptorSetLayout;
	return descriptorSetLayout;
}

DescriptorSetLayout* VulkanRenderer::getDescriptorSetLayout(std::string name)
{
	return descriptorSetLayouts[name];
}
