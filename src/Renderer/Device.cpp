/* Copyright (c) 2020-2023, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Device.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

Device::Device(VkPhysicalDevice physicalDevice, Instance* instance)
{
	assert(physicalDevice);
	assert(instance);
	this->physicalDevice = physicalDevice;
	this->instance = instance;

	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	assert(queueFamilyCount > 0);
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	// Get list of supported extensions
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (auto& ext : extensions)
			{
				supportedExtensions.push_back(ext.extensionName);
			}
		}
	}
}

Device::~Device()
{
	if (handle)
	{
		vkDestroyDevice(handle, nullptr);
		vmaDestroyAllocator(vmaAllocator);
	}
}

uint32_t Device::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

uint32_t Device::getQueueFamilyIndex(VkQueueFlagBits queueFlags)
{
	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				return i;
				break;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return i;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
	{
		if (queueFamilyProperties[i].queueFlags & queueFlags)
		{
			return i;
			break;
		}
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

VkResult Device::create(VkQueueFlags requestedQueueTypes)
{
	// Desired queues need to be requested upon logical device creation
	// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
	// requests different queue types

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority(0.0f);

	// Graphics queue
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}
	else
	{
		queueFamilyIndices.graphics = VK_NULL_HANDLE;
	}

	// Dedicated compute queue
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
		if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		queueFamilyIndices.compute = queueFamilyIndices.graphics;
	}

	// Dedicated transfer queue
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
		if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		queueFamilyIndices.transfer = queueFamilyIndices.graphics;
	}

	// Create the logical device representation
	std::vector<const char*> deviceExtensions(enabledExtensions);
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	// If a pNext(Chain) has been passed, we need to add it to the device creation info
	if (pNext) {
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = enabledFeatures;
		physicalDeviceFeatures2.pNext = pNext;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.pNext = &physicalDeviceFeatures2;
	}

	// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
	if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	}

	deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

	if (deviceExtensions.size() > 0)
	{
		deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &handle);

	if (result == VK_SUCCESS)
	{
		commandPool = createCommandPool(queueFamilyIndices.graphics);
	}

	this->enabledFeatures = enabledFeatures;

	// Initialize Vulkan Memory allocator
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.device = handle;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.instance = instance->handle;
	vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

	return result;
}

void Device::setDebugObjectName(VkObjectType type, uint64_t handle, const char* name)
{
	if (!vks::debug::debugUtilsAvailable) {
		return;
	}
	VkDebugUtilsObjectNameInfoEXT nameInfo{};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = type;
	nameInfo.objectHandle = handle;
	nameInfo.pObjectName = name;
	vks::debug::vkSetDebugUtilsObjectNameEXT(this->handle, &nameInfo);
}

VkResult Device::createBuffer(VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, Buffer* buffer, VkDeviceSize size, void* data)
{
	VkBufferCreateInfo bufferCI{};
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCI.usage = usageFlags;
	bufferCI.size = size;

	VmaAllocationCreateInfo allocCI{};
	allocCI.usage = memoryUsage;
	allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo allocInfo{};
	VK_CHECK_RESULT(vmaCreateBuffer(vmaAllocator, &bufferCI, &allocCI, &buffer->buffer, &buffer->vmaAllocation, &allocInfo));

	buffer->mapped = allocInfo.pMappedData;
	buffer->vmaAllocator = &vmaAllocator;

	if (data != nullptr)
	{
		memcpy(buffer->mapped, data, size);
		// @todo: flush non-coherent
	}

	buffer->setupDescriptor();

	return VK_SUCCESS;
}

void Device::copyBuffer(Buffer* src, Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion)
{
	assert(dst->size <= src->size);
	assert(src->buffer);
	VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy bufferCopy{};
	if (copyRegion == nullptr)
	{
		bufferCopy.size = src->size;
	}
	else
	{
		bufferCopy = *copyRegion;
	}

	vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

	flushCommandBuffer(copyCmd, queue);
}

VkCommandPool Device::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	cmdPoolInfo.flags = createFlags;
	VkCommandPool cmdPool;
	VK_CHECK_RESULT(vkCreateCommandPool(handle, &cmdPoolInfo, nullptr, &cmdPool));
	return cmdPool;
}

VkCommandBuffer Device::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, level, 1);

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(handle, &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}

	return cmdBuffer;
}

void Device::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = vks::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(handle, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(handle, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(handle, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(handle, commandPool, 1, &commandBuffer);
	}
}

bool Device::extensionSupported(std::string extension)
{
	return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
}

void Device::enableExtension(const char* extension)
{
	enabledExtensions.push_back(extension);
}

void Device::setPNext(void* pNext)
{
	this->pNext = pNext;
}
