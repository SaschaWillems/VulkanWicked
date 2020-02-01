/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Sampler.h"

Sampler::Sampler(Device* device)
{
    this->device = device;
}

Sampler::~Sampler()
{
    vkDestroySampler(device->handle, handle, nullptr);
}

void Sampler::create()
{
	VkSamplerCreateInfo CI{};
	CI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    CI.addressModeU = addressModeU;
    CI.addressModeV = addressModeV;
    CI.addressModeW = addressModeW;
    CI.borderColor = borderColor;
    CI.minFilter = minFilter;
    CI.magFilter = magFilter;
    CI.maxAnisotropy = maxAnisotropy;
    CI.anisotropyEnable = (maxAnisotropy > 0.0f);
    VK_CHECK_RESULT(vkCreateSampler(device->handle, &CI, nullptr, &handle));
}

void Sampler::setMinFilter(VkFilter minFilter)
{
    this->minFilter = minFilter;
}

void Sampler::setMagFilter(VkFilter magFilter)
{
    this->magFilter = magFilter;
}

void Sampler::setAddressMode(VkSamplerAddressMode adressMode)
{
    this->addressModeU = adressMode;
    this->addressModeV = adressMode;
    this->addressModeW = adressMode;
}

void Sampler::setAddressModeU(VkSamplerAddressMode adressMode)
{
    this->addressModeU = adressMode;
}

void Sampler::setAddressModeV(VkSamplerAddressMode adressMode)
{
    this->addressModeV = adressMode;
}

void Sampler::setAddressModeW(VkSamplerAddressMode adressMode)
{
    this->addressModeW = adressMode;
}

void Sampler::setBorderColor(VkBorderColor borderColor)
{
    this->borderColor = borderColor;
}

void Sampler::setMaxAnisotropy(float maxAnisotropy)
{
    this->maxAnisotropy = maxAnisotropy;
}



