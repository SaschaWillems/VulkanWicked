/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "VulkanglTFModel.h"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

namespace vkglTF
{
	PushConstBlockMaterial pushConstBlockMaterial;

	BoundingBox BoundingBox::getAABB(glm::mat4 m) {
		glm::vec3 min = glm::vec3(m[3]);
		glm::vec3 max = min;
		glm::vec3 v0, v1;

		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * this->min.x;
		v1 = right * this->max.x;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * this->min.y;
		v1 = up * this->max.y;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * this->min.z;
		v1 = back * this->max.z;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		return BoundingBox(min, max);
	}

		void Texture::updateDescriptor()
		{
			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;
		}

		void Texture::destroy()
		{
			vkDestroyImageView(device->handle, view, nullptr);
			vkDestroyImage(device->handle, image, nullptr);
			vkFreeMemory(device->handle, deviceMemory, nullptr);
			vkDestroySampler(device->handle, sampler, nullptr);
		}

		/*
			Load a texture from a glTF image (stored as vector of chars loaded via stb_image)
			Also generates the mip chain as glTF images are stored as jpg or png without any mips
		*/
		void Texture::fromglTfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler, Device* device, VkQueue copyQueue)
		{
			this->device = device;

			unsigned char* buffer = nullptr;
			VkDeviceSize bufferSize = 0;
			bool deleteBuffer = false;
			if (gltfimage.component == 3) {
				// Most devices don't support RGB only on Vulkan so convert if necessary
				// TODO: Check actual format support and transform only if required
				bufferSize = gltfimage.width * gltfimage.height * 4;
				buffer = new unsigned char[bufferSize];
				unsigned char* rgba = buffer;
				unsigned char* rgb = &gltfimage.image[0];
				for (int32_t i = 0; i < gltfimage.width * gltfimage.height; ++i) {
					for (int32_t j = 0; j < 3; ++j) {
						rgba[j] = rgb[j];
					}
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else {
				buffer = &gltfimage.image[0];
				bufferSize = gltfimage.image.size();
			}

			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

			VkFormatProperties formatProperties;

			width = gltfimage.width;
			height = gltfimage.height;
			mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
			assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
			assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs{};

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK_CHECK_RESULT(vkCreateBuffer(device->handle, &bufferCreateInfo, nullptr, &stagingBuffer));
			vkGetBufferMemoryRequirements(device->handle, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->handle, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device->handle, stagingBuffer, stagingMemory, 0));

			uint8_t* data;
			VK_CHECK_RESULT(vkMapMemory(device->handle, stagingMemory, 0, memReqs.size, 0, (void**)&data));
			memcpy(data, buffer, bufferSize);
			vkUnmapMemory(device->handle, stagingMemory);

			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(device->handle, &imageCreateInfo, nullptr, &image));
			vkGetImageMemoryRequirements(device->handle, image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->handle, &memAllocInfo, nullptr, &deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->handle, image, deviceMemory, 0));

			VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			device->flushCommandBuffer(copyCmd, copyQueue, true);

			vkFreeMemory(device->handle, stagingMemory, nullptr);
			vkDestroyBuffer(device->handle, stagingBuffer, nullptr);

			// Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
			VkCommandBuffer blitCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			for (uint32_t i = 1; i < mipLevels; i++) {
				VkImageBlit imageBlit{};

				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstOffsets[1].x = int32_t(width >> i);
				imageBlit.dstOffsets[1].y = int32_t(height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = 0;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.image = image;
					imageMemoryBarrier.subresourceRange = mipSubRange;
					vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}

				vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.image = image;
					imageMemoryBarrier.subresourceRange = mipSubRange;
					vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
			}

			subresourceRange.levelCount = mipLevels;
			imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			device->flushCommandBuffer(blitCmd, copyQueue, true);

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = textureSampler.magFilter;
			samplerInfo.minFilter = textureSampler.minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = textureSampler.addressModeU;
			samplerInfo.addressModeV = textureSampler.addressModeV;
			samplerInfo.addressModeW = textureSampler.addressModeW;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerInfo.maxAnisotropy = 1.0;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxLod = (float)mipLevels;
			samplerInfo.maxAnisotropy = 8.0f;
			samplerInfo.anisotropyEnable = VK_TRUE;
			VK_CHECK_RESULT(vkCreateSampler(device->handle, &samplerInfo, nullptr, &sampler));

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.levelCount = mipLevels;
			VK_CHECK_RESULT(vkCreateImageView(device->handle, &viewInfo, nullptr, &view));

			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;

			if (deleteBuffer)
				delete[] buffer;

		}

		Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
			hasIndices = indexCount > 0;
		};

		void Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max) {
			bb.min = min;
			bb.max = max;
			bb.valid = true;
		}
	

		Mesh::Mesh(Device* device, glm::mat4 matrix) {
			this->device = device;
			this->uniformBlock.matrix = matrix;
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				&uniformBuffer.buffer,
				sizeof(uniformBlock),
				&uniformBlock));
		};

		Mesh::~Mesh() {
			uniformBuffer.buffer.destroy();
			for (Primitive* p : primitives)
				delete p;
		}

		void Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max) {
			bb.min = min;
			bb.max = max;
			bb.valid = true;
		}

		glm::mat4 Node::localMatrix() {
			return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
		}

		glm::mat4 Node::getMatrix() {
			glm::mat4 m = localMatrix();
			vkglTF::Node* p = parent;
			while (p) {
				m = p->localMatrix() * m;
				p = p->parent;
			}
			return m;
		}

		void Node::update() {
			if (mesh) {
				glm::mat4 m = getMatrix();
				if (skin) {
					mesh->uniformBlock.matrix = m;
					// Update join matrices
					glm::mat4 inverseTransform = glm::inverse(m);
					size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
					for (size_t i = 0; i < numJoints; i++) {
						vkglTF::Node* jointNode = skin->joints[i];
						glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
						jointMat = inverseTransform * jointMat;
						mesh->uniformBlock.jointMatrix[i] = jointMat;
					}
					mesh->uniformBlock.jointcount = (float)numJoints;
					memcpy(mesh->uniformBuffer.buffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
				}
				else {
					memcpy(mesh->uniformBuffer.buffer.mapped, &m, sizeof(glm::mat4));
				}
			}

			for (auto& child : children) {
				child->update();
			}
		}

		Node::~Node() {
			if (mesh) {
				delete mesh;
			}
			for (auto& child : children) {
				delete child;
			}
		}

	/*
		glTF animation sampler
	*/

		// Details on how this works can be found in the specs: 
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
		glm::vec4 AnimationSampler::cubicSplineInterpolation(size_t index, float time, uint32_t stride)
		{
			auto delta = inputs[index + 1] - inputs[index];
			auto t = (time - inputs[index]) / delta;
			const uint32_t current = index * stride * 3;
			const uint32_t next = (index + 1) * stride * 3;
			const uint32_t A = 0;
			const uint32_t V = stride * 1;
			const uint32_t B = stride * 2;

			float t2 = pow(t, 2);
			float t3 = pow(t, 3);
			glm::vec4 pt;
			for (uint32_t i = 0; i < stride; i++) {
				float p0 = outputs[current + i + V];			// starting point at t = 0
				float m0 = delta * outputs[current + i + A];	// scaled starting tangent at t = 0
				float p1 = outputs[next + i + V];				// ending point at t = 1
				float m1 = delta * outputs[next + i + B];		// scaled ending tangent at t = 1
				pt[i] = ((2.0 * t3 - 3.0 * t2 + 1.0) * p0) + ((t3 - 2 * t2 + t) * m0) + ((-2 * t3 + 3 * t2) * p1) + ((t3 - t2) * m0);
			}
			return pt;
		}

		void AnimationSampler::translate(size_t index, float time, vkglTF::Node* node)
		{
			switch (interpolation) {
			case AnimationSampler::InterpolationType::LINEAR: {
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				node->translation = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
				break;
			}
			case AnimationSampler::InterpolationType::STEP: {
				node->translation = outputsVec4[index];
				break;
			}
			case AnimationSampler::InterpolationType::CUBICSPLINE: {
				node->translation = cubicSplineInterpolation(index, time, 3);
				break;
			}
			}
		}

		void AnimationSampler::scale(size_t index, float time, vkglTF::Node* node) {
			switch (interpolation) {
			case AnimationSampler::InterpolationType::LINEAR: {
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				node->scale = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
				break;
			}
			case AnimationSampler::InterpolationType::STEP: {
				node->scale = outputsVec4[index];
				break;
			}
			case AnimationSampler::InterpolationType::CUBICSPLINE: {
				node->scale = cubicSplineInterpolation(index, time, 3);
				break;
			}
			}
		}

		void AnimationSampler::rotate(size_t index, float time, vkglTF::Node* node) {
			switch (interpolation) {
			case AnimationSampler::InterpolationType::LINEAR: {
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				glm::quat q1;
				q1.x = outputsVec4[index].x;
				q1.y = outputsVec4[index].y;
				q1.z = outputsVec4[index].z;
				q1.w = outputsVec4[index].w;
				glm::quat q2;
				q2.x = outputsVec4[index + 1].x;
				q2.y = outputsVec4[index + 1].y;
				q2.z = outputsVec4[index + 1].z;
				q2.w = outputsVec4[index + 1].w;
				node->rotation = glm::normalize(glm::slerp(q1, q2, u));
				break;
			}
			case AnimationSampler::InterpolationType::STEP: {
				glm::quat q1;
				q1.x = outputsVec4[index].x;
				q1.y = outputsVec4[index].y;
				q1.z = outputsVec4[index].z;
				q1.w = outputsVec4[index].w;
				node->rotation = q1;
				break;
			}
			case AnimationSampler::InterpolationType::CUBICSPLINE: {
				glm::vec4 rot = cubicSplineInterpolation(index, time, 4);
				glm::quat q;
				q.x = rot.x;
				q.y = rot.y;
				q.z = rot.z;
				q.w = rot.w;
				node->rotation = glm::normalize(q);
				break;
			}
			}
		}

	
		void Model::destroy(VkDevice device)
		{
			if (vertices.buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(device, vertices.buffer, nullptr);
				vkFreeMemory(device, vertices.memory, nullptr);
			}
			if (indices.buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(device, indices.buffer, nullptr);
				vkFreeMemory(device, indices.memory, nullptr);
			}
			for (auto texture : textures) {
				texture.destroy();
			}
			textures.resize(0);
			textureSamplers.resize(0);
			for (auto node : nodes) {
				delete node;
			}
			materials.resize(0);
			animations.resize(0);
			nodes.resize(0);
			linearNodes.resize(0);
			extensions.resize(0);
			skins.resize(0);
		};

		void Model::loadNode(vkglTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale)
		{
			vkglTF::Node* newNode = new Node{};
			newNode->index = nodeIndex;
			newNode->parent = parent;
			newNode->name = node.name;
			newNode->skinIndex = node.skin;
			newNode->matrix = glm::mat4(1.0f);

			// Generate local node matrix
			glm::vec3 translation = glm::vec3(0.0f);
			if (node.translation.size() == 3) {
				translation = glm::make_vec3(node.translation.data());
				newNode->translation = translation;
			}
			glm::mat4 rotation = glm::mat4(1.0f);
			if (node.rotation.size() == 4) {
				glm::quat q = glm::make_quat(node.rotation.data());
				newNode->rotation = glm::mat4(q);
			}
			glm::vec3 scale = glm::vec3(1.0f);
			if (node.scale.size() == 3) {
				scale = glm::make_vec3(node.scale.data());
				newNode->scale = scale;
			}
			if (node.matrix.size() == 16) {
				newNode->matrix = glm::make_mat4x4(node.matrix.data());
			};

			// Node with children
			if (node.children.size() > 0) {
				for (size_t i = 0; i < node.children.size(); i++) {
					loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
				}
			}

			// Node contains mesh data
			if (node.mesh > -1) {
				const tinygltf::Mesh mesh = model.meshes[node.mesh];
				Mesh* newMesh = new Mesh(device, newNode->matrix);
				for (size_t j = 0; j < mesh.primitives.size(); j++) {
					const tinygltf::Primitive& primitive = mesh.primitives[j];
					uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
					uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
					uint32_t indexCount = 0;
					uint32_t vertexCount = 0;
					glm::vec3 posMin{};
					glm::vec3 posMax{};
					bool hasSkin = false;
					bool hasIndices = primitive.indices > -1;
					// Vertices
					{
						const float* bufferPos = nullptr;
						const float* bufferNormals = nullptr;
						const float* bufferTexCoordSet0 = nullptr;
						const float* bufferTexCoordSet1 = nullptr;
						const uint16_t* bufferJoints = nullptr;
						const float* bufferWeights = nullptr;

						// Position attribute is required
						assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

						const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
						bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
						posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
						posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
						vertexCount = static_cast<uint32_t>(posAccessor.count);

						if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
							const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
							const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
							bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
						}

						if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
							const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
							const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
							bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}
						if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
							const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
							const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
							bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}

						// Skinning
						// Joints
						if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
							const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
							const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
							bufferJoints = reinterpret_cast<const uint16_t*>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
						}

						if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
							const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
							const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
							bufferWeights = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}

						hasSkin = (bufferJoints && bufferWeights);

						for (size_t v = 0; v < posAccessor.count; v++) {
							Vertex vert{};
							vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
							vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
							vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * 2]) : glm::vec3(0.0f);
							vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * 2]) : glm::vec3(0.0f);

							vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
							vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
							vertexBuffer.push_back(vert);
						}
					}
					// Indices
					if (hasIndices)
					{
						const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
						const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

						indexCount = static_cast<uint32_t>(accessor.count);
						const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

						switch (accessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						default:
							std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
							return;
						}
					}
					Primitive* newPrimitive = new Primitive(indexStart, indexCount, vertexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
					newPrimitive->setBoundingBox(posMin, posMax);
					newMesh->primitives.push_back(newPrimitive);
				}
				// Mesh BB from BBs of primitives
				for (auto p : newMesh->primitives) {
					if (p->bb.valid && !newMesh->bb.valid) {
						newMesh->bb = p->bb;
						newMesh->bb.valid = true;
					}
					newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
					newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
				}
				newNode->mesh = newMesh;
			}
			if (parent) {
				parent->children.push_back(newNode);
			}
			else {
				nodes.push_back(newNode);
			}
			linearNodes.push_back(newNode);
		}

		void Model::loadSkins(tinygltf::Model& gltfModel)
		{
			for (tinygltf::Skin& source : gltfModel.skins) {
				Skin* newSkin = new Skin{};
				newSkin->name = source.name;

				// Find skeleton root node
				if (source.skeleton > -1) {
					newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
				}

				// Find joint nodes
				for (int jointIndex : source.joints) {
					Node* node = nodeFromIndex(jointIndex);
					if (node) {
						newSkin->joints.push_back(nodeFromIndex(jointIndex));
					}
				}

				// Get inverse bind matrices from buffer
				if (source.inverseBindMatrices > -1) {
					const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
					const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
					newSkin->inverseBindMatrices.resize(accessor.count);
					memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
				}

				skins.push_back(newSkin);
			}
		}

		void Model::loadTextures(tinygltf::Model& gltfModel, Device* device, VkQueue transferQueue)
		{
			for (tinygltf::Texture& tex : gltfModel.textures) {
				tinygltf::Image image = gltfModel.images[tex.source];
				vkglTF::TextureSampler textureSampler;
				if (tex.sampler == -1) {
					// No sampler specified, use a default one
					textureSampler.magFilter = VK_FILTER_LINEAR;
					textureSampler.minFilter = VK_FILTER_LINEAR;
					textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				}
				else {
					textureSampler = textureSamplers[tex.sampler];
				}
				vkglTF::Texture texture;
				texture.fromglTfImage(image, textureSampler, device, transferQueue);
				textures.push_back(texture);
			}
		}

		VkSamplerAddressMode Model::getVkWrapMode(int32_t wrapMode)
		{
			switch (wrapMode) {
			case 10497:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case 33071:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case 33648:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			}
		}

		VkFilter Model::getVkFilterMode(int32_t filterMode)
		{
			switch (filterMode) {
			case 9728:
				return VK_FILTER_NEAREST;
			case 9729:
				return VK_FILTER_LINEAR;
			case 9984:
				return VK_FILTER_NEAREST;
			case 9985:
				return VK_FILTER_NEAREST;
			case 9986:
				return VK_FILTER_LINEAR;
			case 9987:
				return VK_FILTER_LINEAR;
			}
		}

		void Model::loadTextureSamplers(tinygltf::Model& gltfModel)
		{
			for (tinygltf::Sampler smpl : gltfModel.samplers) {
				vkglTF::TextureSampler sampler{};
				sampler.minFilter = getVkFilterMode(smpl.minFilter);
				sampler.magFilter = getVkFilterMode(smpl.magFilter);
				sampler.addressModeU = getVkWrapMode(smpl.wrapS);
				sampler.addressModeV = getVkWrapMode(smpl.wrapT);
				sampler.addressModeW = sampler.addressModeV;
				textureSamplers.push_back(sampler);
			}
		}

		void Model::loadMaterials(tinygltf::Model& gltfModel)
		{
			for (tinygltf::Material& mat : gltfModel.materials) {
				vkglTF::Material material{};
				if (mat.values.find("baseColorTexture") != mat.values.end()) {
					material.baseColorTexture = &textures[mat.values["baseColorTexture"].TextureIndex()];
					material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
				}
				if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
					material.metallicRoughnessTexture = &textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
					material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
				}
				if (mat.values.find("roughnessFactor") != mat.values.end()) {
					material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
				}
				if (mat.values.find("metallicFactor") != mat.values.end()) {
					material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
				}
				if (mat.values.find("baseColorFactor") != mat.values.end()) {
					material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
				}
				if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
					material.normalTexture = &textures[mat.additionalValues["normalTexture"].TextureIndex()];
					material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
				}
				if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
					material.emissiveTexture = &textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
					material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
				}
				if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
					material.occlusionTexture = &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
					material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
				}
				if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
					tinygltf::Parameter param = mat.additionalValues["alphaMode"];
					if (param.string_value == "BLEND") {
						material.alphaMode = Material::ALPHAMODE_BLEND;
					}
					if (param.string_value == "MASK") {
						material.alphaCutoff = 0.5f;
						material.alphaMode = Material::ALPHAMODE_MASK;
					}
				}
				if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
					material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
				}
				if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
					material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
					material.emissiveFactor = glm::vec4(0.0f);
				}

				// Extensions
				// @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
				if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
					auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
					if (ext->second.Has("specularGlossinessTexture")) {
						auto index = ext->second.Get("specularGlossinessTexture").Get("index");
						material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
						auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
						material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
						material.pbrWorkflows.specularGlossiness = true;
					}
					if (ext->second.Has("diffuseTexture")) {
						auto index = ext->second.Get("diffuseTexture").Get("index");
						material.extension.diffuseTexture = &textures[index.Get<int>()];
					}
					if (ext->second.Has("diffuseFactor")) {
						auto factor = ext->second.Get("diffuseFactor");
						for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
							auto val = factor.Get(i);
							material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
						}
					}
					if (ext->second.Has("specularFactor")) {
						auto factor = ext->second.Get("specularFactor");
						for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
							auto val = factor.Get(i);
							material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
						}
					}
				}

				materials.push_back(material);
			}
			// Push a default material at the end of the list for meshes with no material assigned
			materials.push_back(Material());
		}

		void Model::loadAnimations(tinygltf::Model& gltfModel)
		{
			for (tinygltf::Animation& anim : gltfModel.animations) {
				vkglTF::Animation animation{};
				animation.name = anim.name;
				if (anim.name.empty()) {
					animation.name = std::to_string(animations.size());
				}

				// Samplers
				for (auto& samp : anim.samplers) {
					vkglTF::AnimationSampler sampler{};

					if (samp.interpolation == "LINEAR") {
						sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
					}
					if (samp.interpolation == "STEP") {
						sampler.interpolation = AnimationSampler::InterpolationType::STEP;
					}
					if (samp.interpolation == "CUBICSPLINE") {
						sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
					}

					// Read sampler input time values
					{
						const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

						const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
						const float* buf = static_cast<const float*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.inputs.push_back(buf[index]);
						}

						for (auto input : sampler.inputs) {
							if (input < animation.start) {
								animation.start = input;
							};
							if (input > animation.end) {
								animation.end = input;
							}
						}
					}

					// Read sampler output T/R/S values 
					{
						const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

						const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

						switch (accessor.type) {
						case TINYGLTF_TYPE_VEC3: {
							const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
								sampler.outputs.push_back(buf[index][0]);
								sampler.outputs.push_back(buf[index][1]);
								sampler.outputs.push_back(buf[index][2]);
							}
							break;
						}
						case TINYGLTF_TYPE_VEC4: {
							const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								sampler.outputsVec4.push_back(buf[index]);
								sampler.outputs.push_back(buf[index][0]);
								sampler.outputs.push_back(buf[index][1]);
								sampler.outputs.push_back(buf[index][2]);
								sampler.outputs.push_back(buf[index][3]);
							}
							break;
						}
						default: {
							std::cout << "unknown type" << std::endl;
							break;
						}
						}
					}

					animation.samplers.push_back(sampler);
				}

				// Channels
				for (auto& source : anim.channels) {
					vkglTF::AnimationChannel channel{};

					if (source.target_path == "rotation") {
						channel.path = AnimationChannel::PathType::ROTATION;
					}
					if (source.target_path == "translation") {
						channel.path = AnimationChannel::PathType::TRANSLATION;
					}
					if (source.target_path == "scale") {
						channel.path = AnimationChannel::PathType::SCALE;
					}
					if (source.target_path == "weights") {
						std::cout << "weights not yet supported, skipping channel" << std::endl;
						continue;
					}
					channel.samplerIndex = source.sampler;
					channel.node = nodeFromIndex(source.target_node);
					if (!channel.node) {
						continue;
					}

					animation.channels.push_back(channel);
				}

				animations.push_back(animation);
			}
		}

		void Model::loadFromFile(std::string filename, Device* device, VkQueue transferQueue, float scale)
		{
			tinygltf::Model gltfModel;
			tinygltf::TinyGLTF gltfContext;
			std::string error;
			std::string warning;

			this->device = device;

			bool binary = false;
			size_t extpos = filename.rfind('.', filename.length());
			if (extpos != std::string::npos) {
				binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
			}

			bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str()) : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

			std::vector<uint32_t> indexBuffer;
			std::vector<Vertex> vertexBuffer;

			if (fileLoaded) {
				loadTextureSamplers(gltfModel);
				loadTextures(gltfModel, device, transferQueue);
				loadMaterials(gltfModel);
				// TODO: scene handling with no default scene
				const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
				for (size_t i = 0; i < scene.nodes.size(); i++) {
					const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
					loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
				}
				if (gltfModel.animations.size() > 0) {
					loadAnimations(gltfModel);
				}
				loadSkins(gltfModel);

				for (auto node : linearNodes) {
					// Assign skins
					if (node->skinIndex > -1) {
						node->skin = skins[node->skinIndex];
					}
					// Initial pose
					if (node->mesh) {
						node->update();
					}
				}
			}
			else {
				// TODO: throw
				std::cerr << "Could not load gltf file: " << error << std::endl;
				return;
			}

			extensions = gltfModel.extensionsUsed;

			size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
			size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
			indexCount = static_cast<uint32_t>(indexBuffer.size());

			assert(vertexBufferSize > 0);

			Buffer vertexStaging, indexStaging;

			// Create staging buffers
			// Vertex data
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY,
				&vertexStaging,
				vertexBufferSize,
				vertexBuffer.data()));
			// Index data
			if (indexBufferSize > 0) {
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VMA_MEMORY_USAGE_CPU_ONLY,
					&indexStaging,
					indexBufferSize,
					indexBuffer.data()));
			}

			// Create device local buffers
			// Vertex buffer
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				&vertices,
				vertexBufferSize));
			// Index buffer
			if (indexBufferSize > 0) {
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_GPU_ONLY,
					&indices,
					indexBufferSize));
			}

			// Copy from staging buffers
			VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

			if (indexBufferSize > 0) {
				copyRegion.size = indexBufferSize;
				vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
			}

			device->flushCommandBuffer(copyCmd, transferQueue, true);

			vkDestroyBuffer(device->handle, vertexStaging.buffer, nullptr);
			vkFreeMemory(device->handle, vertexStaging.memory, nullptr);
			if (indexBufferSize > 0) {
				vkDestroyBuffer(device->handle, indexStaging.buffer, nullptr);
				vkFreeMemory(device->handle, indexStaging.memory, nullptr);
			}

			getSceneDimensions();
		}

		void Model::drawNode(Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstInstance)
		{
			if (node->mesh) {
				for (Primitive* primitive : node->mesh->primitives) {

					// Pass material parameters as push constants
					PushConstBlockMaterial pushConstBlockMaterial{};
					pushConstBlockMaterial.emissiveFactor = primitive->material.emissiveFactor;
					// To save push constant space, availabilty and texture coordiante set are combined
					// -1 = texture not used for this material, >= 0 texture used and index of texture coordinate set
					pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
					pushConstBlockMaterial.normalTextureSet = primitive->material.normalTexture != nullptr ? primitive->material.texCoordSets.normal : -1;
					pushConstBlockMaterial.occlusionTextureSet = primitive->material.occlusionTexture != nullptr ? primitive->material.texCoordSets.occlusion : -1;
					pushConstBlockMaterial.emissiveTextureSet = primitive->material.emissiveTexture != nullptr ? primitive->material.texCoordSets.emissive : -1;
					pushConstBlockMaterial.alphaMask = static_cast<float>(primitive->material.alphaMode == vkglTF::Material::ALPHAMODE_MASK);
					pushConstBlockMaterial.alphaMaskCutoff = primitive->material.alphaCutoff;

					// TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

					if (primitive->material.pbrWorkflows.metallicRoughness) {
						// Metallic roughness workflow
						pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_METALLIC_ROUGHNESS);
						pushConstBlockMaterial.baseColorFactor = primitive->material.baseColorFactor;
						pushConstBlockMaterial.metallicFactor = primitive->material.metallicFactor;
						pushConstBlockMaterial.roughnessFactor = primitive->material.roughnessFactor;
						pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.metallicRoughnessTexture != nullptr ? primitive->material.texCoordSets.metallicRoughness : -1;
						pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
					}

					if (primitive->material.pbrWorkflows.specularGlossiness) {
						// Specular glossiness workflow
						pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_SPECULAR_GLOSINESS);
						pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.extension.specularGlossinessTexture != nullptr ? primitive->material.texCoordSets.specularGlossiness : -1;
						pushConstBlockMaterial.colorTextureSet = primitive->material.extension.diffuseTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
						pushConstBlockMaterial.diffuseFactor = primitive->material.extension.diffuseFactor;
						pushConstBlockMaterial.specularFactor = glm::vec4(primitive->material.extension.specularFactor, 1.0f);
					}

					// @todo: Setter for material push constant offset, size, layout etc.
					vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlockMaterial), &pushConstBlockMaterial);

					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, firstInstance);
				}
			}
			for (auto& child : node->children) {
				drawNode(child, commandBuffer, pipelineLayout);
			}
		}

		void Model::bindBuffers(VkCommandBuffer commandBuffer)
		{
			const VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		}

		void Model::drawNodes(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstInstance)
		{
			for (auto& node : nodes) {
				drawNode(node, commandBuffer, pipelineLayout, firstInstance);
			}
		}

		void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstInstance)
		{
			const VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			for (auto& node : nodes) {
				drawNode(node, commandBuffer, pipelineLayout, firstInstance);
			}
		}

		void Model::calculateBoundingBox(Node* node, Node* parent) {
			BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

			if (node->mesh) {
				if (node->mesh->bb.valid) {
					node->aabb = node->mesh->bb.getAABB(node->getMatrix());
					if (node->children.size() == 0) {
						node->bvh.min = node->aabb.min;
						node->bvh.max = node->aabb.max;
						node->bvh.valid = true;
					}
				}
			}

			parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
			parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

			for (auto& child : node->children) {
				calculateBoundingBox(child, node);
			}
		}

		void Model::getSceneDimensions()
		{
			// Calculate binary volume hierarchy for all nodes in the scene
			for (auto node : linearNodes) {
				calculateBoundingBox(node, nullptr);
			}

			dimensions.min = glm::vec3(FLT_MAX);
			dimensions.max = glm::vec3(-FLT_MAX);

			for (auto node : linearNodes) {
				if (node->bvh.valid) {
					dimensions.min = glm::min(dimensions.min, node->bvh.min);
					dimensions.max = glm::max(dimensions.max, node->bvh.max);
				}
			}

			// Calculate scene aabb
			aabb = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
			aabb[3][0] = dimensions.min[0];
			aabb[3][1] = dimensions.min[1];
			aabb[3][2] = dimensions.min[2];
		}

		void Model::updateAnimation(uint32_t index, float time)
		{
			if (index > static_cast<uint32_t>(animations.size()) - 1) {
				std::cout << "No animation with index " << index << std::endl;
				return;
			}
			Animation& animation = animations[index];

			bool updated = false;
			for (auto& channel : animation.channels) {
				vkglTF::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
				if (sampler.inputs.size() > sampler.outputsVec4.size()) {
					continue;
				}

				for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
					if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
						float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
						if (u <= 1.0f) {
							switch (channel.path) {
							case vkglTF::AnimationChannel::PathType::TRANSLATION:
								sampler.translate(i, time, channel.node);
								break;
							case vkglTF::AnimationChannel::PathType::SCALE:
								sampler.scale(i, time, channel.node);
								break;
							case vkglTF::AnimationChannel::PathType::ROTATION:
								sampler.rotate(i, time, channel.node);
							}
							updated = true;
						}
					}
				}
			}
			if (updated) {
				for (auto& node : nodes) {
					node->update();
				}
			}
		}

		/*
			Helper functions
		*/
		Node* Model::findNode(Node* parent, uint32_t index) {
			Node* nodeFound = nullptr;
			if (parent->index == index) {
				return parent;
			}
			for (auto& child : parent->children) {
				nodeFound = findNode(child, index);
				if (nodeFound) {
					break;
				}
			}
			return nodeFound;
		}

		Node* Model::nodeFromIndex(uint32_t index) {
			Node* nodeFound = nullptr;
			for (auto& node : nodes) {
				nodeFound = findNode(node, index);
				if (nodeFound) {
					break;
				}
			}
			return nodeFound;
		}
}