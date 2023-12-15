#pragma once
#include "vulkan/vulkan.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif
#ifdef __ANDROID__
#include "VulkanAndroid.h"
#endif
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vks
{
	namespace debug
	{
		// Default validation layers
		extern bool debugUtilsAvailable;
		extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

		// Default debug callback
		VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t srcObject,
			size_t location,
			int32_t msgCode,
			const char* pLayerPrefix,
			const char* pMsg,
			void* pUserData);

		// Load debug function pointers and set debug callback
		// if callBack is NULL, default message callback will be used
		void setupDebugging(
			VkInstance instance,
			VkDebugReportFlagsEXT flags,
			VkDebugReportCallbackEXT callBack);
		// Clear debug callback
		void freeDebugCallback(VkInstance instance);
	}
}
