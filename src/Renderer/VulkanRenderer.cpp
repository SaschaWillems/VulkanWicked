/* Copyright (c) 2020, Sascha Willems
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
	}

#if defined(_WIN32)
	// Enable console if validation is active
	// Debug message callback will output to it
	if (this->settings.validation)
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w+", stdout);
		freopen_s(&stream, "CONOUT$", "w+", stderr);
	}
#endif

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return;
	}
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

	device = new Device(physicalDevice);
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
	setupPipelines();
	setupDescriptorPool();

	// Deferred composition
	VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &deferredComposition.lightsBuffer, sizeof(lightSources)));
	VK_CHECK_RESULT(deferredComposition.lightsBuffer.map());

	deferredComposition.descriptorSet = new DescriptorSet(device->handle);
	deferredComposition.descriptorSet->setPool(descriptorPool);
	deferredComposition.descriptorSet->addLayout(getDescriptorSetLayout("deferred_composition"));
	deferredComposition.descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.position.descriptor);
	deferredComposition.descriptorSet->addDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.normal.descriptor);
	deferredComposition.descriptorSet->addDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offscreenPass.albedo.descriptor);
	deferredComposition.descriptorSet->addDescriptor(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &deferredComposition.lightsBuffer.descriptor);
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
	frameBufferCreateInfo.pNext = NULL;
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

void VulkanRenderer::createFrameBufferImage(FrameBufferAttachment& target, FramebufferType type, VkFormat fmt)
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
		{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};

	RenderPass* renderPass = addRenderPass("offscreen");
	renderPass->setDimensions(width, height);

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
	renderPass->setDepthStencilClearValue(3, 1.0f, 0);
	renderPass->create();

	offscreenPass.width = width;
	offscreenPass.height = height;

	/* Renderpass */

	/* Shared sampler */

	VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
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
	createFrameBufferImage(offscreenPass.position, FramebufferType::Color, VK_FORMAT_R16G16B16A16_SFLOAT);
	createFrameBufferImage(offscreenPass.normal, FramebufferType::Color, VK_FORMAT_R16G16B16A16_SFLOAT);
	createFrameBufferImage(offscreenPass.albedo, FramebufferType::Color, VK_FORMAT_R8G8B8A8_UNORM);
	createFrameBufferImage(offscreenPass.depth, FramebufferType::DepthStencil, fbDepthFormat);

	/* Framebuffers */

	std::vector<VkImageView> attachments = {
		offscreenPass.position.view->handle,
		offscreenPass.normal.view->handle,
		offscreenPass.albedo.view->handle,
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

	PipelineLayout* pipelineLayout;
	pipelineLayout = addPipelineLayout("split_ubo");
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addLayout(getDescriptorSetLayout("single_ubo"));
	pipelineLayout->addPushConstantRange(sizeof(vkglTF::PushConstBlockMaterial), 0, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineLayout->create();

	// Deferred composition
	descriptorSetLayout = addDescriptorSetLayout("deferred_composition");
	descriptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	descriptorSetLayout->create();

	pipelineLayout = addPipelineLayout("deferred_composition");
	pipelineLayout->addLayout(getDescriptorSetLayout("deferred_composition"));
	pipelineLayout->create();
}

void VulkanRenderer::setupPipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

	// Vertex bindings and attributes
	std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
		vks::initializers::vertexInputBindingDescription(0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos)),
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal)),
		vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv0)),
	};
	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.pVertexInputState = &vertexInputState;
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;

	Pipeline* pipeline = nullptr;

	// Player
	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
		vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
	};
	colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	colorBlendState.pAttachments = blendAttachmentStates.data();

	pipeline = addPipeline("player");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("split_ubo"));
	pipeline->setRenderPass(getRenderPass("offscreen"));
	pipeline->addShader("shaders/player.vert.spv");
	pipeline->addShader("shaders/default_model.frag.spv");
	pipeline->create();

	// Backdrop
	pipeline = addPipeline("backdrop");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("split_ubo"));
	pipeline->setRenderPass(getRenderPass("offscreen"));
	pipeline->addShader("shaders/backdrop.vert.spv");
	pipeline->addShader("shaders/default_model.frag.spv");
	pipeline->create();

	// Projectile
	pipeline = addPipeline("projectile");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("split_ubo"));
	pipeline->setRenderPass(getRenderPass("offscreen"));
	pipeline->addShader("shaders/projectile.vert.spv");
	pipeline->addShader("shaders/default_model.frag.spv");
	pipeline->create();

	// Spore
	// Per-Spore Data is passed via instanced data
	vertexInputBindings = {
		vks::initializers::vertexInputBindingDescription(0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		vks::initializers::vertexInputBindingDescription(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
	};
	vertexInputAttributes = {
		// Per-Vertex
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos)),
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal)),
		vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv0)),
		// Per-Instance
		vks::initializers::vertexInputAttributeDescription(1, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, pos)),
		vks::initializers::vertexInputAttributeDescription(1, 4, VK_FORMAT_R32_SFLOAT, offsetof(InstanceData, scale)),
	};
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	pipeline = addPipeline("spore");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("split_ubo"));
	pipeline->setRenderPass(getRenderPass("offscreen"));
	pipeline->addShader("shaders/spore.vert.spv");
	pipeline->addShader("shaders/default_model.frag.spv");
	pipeline->create();

	// Deferred composition
	colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	pipelineCI.pVertexInputState = &emptyInputState;

	pipeline = addPipeline("composition");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("deferred_composition"));
	pipeline->setRenderPass(getRenderPass("deferred_composition"));
	pipeline->addShader("shaders/composition.vert.spv");
	pipeline->addShader("shaders/composition.frag.spv");
	pipeline->create();

	pipeline = addPipeline("composition_paused");
	pipeline->setCreateInfo(pipelineCI);
	pipeline->setCache(pipelineCache);
	pipeline->setLayout(getPipelineLayout("deferred_composition"));
	pipeline->setRenderPass(getRenderPass("deferred_composition"));
	pipeline->addShader("shaders/composition.vert.spv");
	pipeline->addShader("shaders/composition_pause.frag.spv");
	pipeline->create();
}

void VulkanRenderer::setupDescriptorPool()
{
	// @todo: proper sizes
	descriptorPool = new DescriptorPool(device->handle);
	descriptorPool->setMaxSets(16);
	descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32);
	descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32);
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
	lightSources.lights[lightSources.numLights] = lightSource;
	lightSources.numLights++;
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
