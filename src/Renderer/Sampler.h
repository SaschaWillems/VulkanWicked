/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.h"
#include "VulkanTools.h"
#include "Device.h"

class Sampler
{
private:
    Device* device;
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    VkBorderColor borderColor;
    float maxAnisotropy;
public:
    VkSampler handle;
    Sampler(Device* device);
    ~Sampler();
    void setMinFilter(VkFilter minFilter);
    void setMagFilter(VkFilter magFilter);
    void setAddressMode(VkSamplerAddressMode adressMode);
    void setAddressModeU(VkSamplerAddressMode adressMode);
    void setAddressModeV(VkSamplerAddressMode adressMode);
    void setAddressModeW(VkSamplerAddressMode adressMode);
    void setBorderColor(VkBorderColor borderColor);
    void setMaxAnisotropy(float maxAnisotropy);
    void create();
};

