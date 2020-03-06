/* Copyright (c) 2020, Sascha Willems
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GameUI.h"

UI::GameUI* gameUI = nullptr;

namespace UI
{
	GameUI::~GameUI()
	{
		for (auto& font : fonts) {
			delete font.second;
		}
		for (auto& element : textElements) {
			delete element.second;
		}
		vertexBuffer.destroy();
	}

	void GameUI::addFont(std::string name)
	{
		std::clog << "Loading font definitions for \"" << name << "\"" << std::endl;
		Font* font = new Font;
		font->load(name, renderer->device, renderer->queue);
		fonts[name] = font;
	}

	void GameUI::setfont(std::string name)
	{
		font = fonts[name];
		assert(font);
	}

	void GameUI::addTextElement(std::string name, std::string text, glm::vec3 position, TextAlignment alignment, glm::vec4 color, bool visible)
	{
		TextElement* textElement = new TextElement;
		textElement->text = text;
		textElement->position = position;
		//textElement->position.x *= (float)renderer->width / (float)renderer->height;
		textElement->color = color;
		textElement->alignment = alignment;
		textElement->visible = visible;
		textElements[name] = textElement;
	}

	TextElement* GameUI::getTextElement(std::string name)
	{
		assert(textElements.count(name) > 0);
		return textElements[name];
	}

	void GameUI::prepareGPUResources()
	{
		for (auto font : fonts) {
			assert(font.second->texture);
			font.second->descriptorSet = new DescriptorSet(renderer->device->handle);
			font.second->descriptorSet->setPool(renderer->descriptorPool);
			font.second->descriptorSet->addLayout(renderer->getDescriptorSetLayout("ui_text"));
			font.second->descriptorSet->addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &font.second->texture->descriptor);
			font.second->descriptorSet->create();
		}
	}

	void GameUI::updateGPUResources()
	{
		//@todo: Only recreate vertex buffer if no. of vertices has changed, otherwise just copy

		if (textElements.size() == 0) {
			return;
		}

		// Get final vector size
		size_t vertexCount = 0;
		for (auto element : textElements)
		{
			if (element.second->visible)
			{
				for (char& c : element.second->text)
				{
					if (c == '\n' || c == ' ') {
						continue;
					}
					vertexCount += 6;
				}
			}
		}
		vertices.resize(vertexCount);
		
		const float ar = (float)renderer->width / (float)renderer->height;

		uint32_t idx = 0;
		for (auto element : textElements)
		{
			if (element.second->visible)
			{
				const glm::vec2 scale = { 0.0025f / ar, 0.0025f };

				float width = 0.0f;
				float height = font->lineHeight * scale.y;

				float posx = element.second->position.x;
				float posy = element.second->position.y;

				const uint32_t idxStartElement = idx;
				for (char& c : element.second->text)
				{
					if (c == '\n') {
						posx = element.second->position.x;
						posy += font->lineHeight * scale.y;
						height += font->lineHeight * scale.y;
						continue;
					}
					FontCharInfo* charInfo = &font->charInfo[(int)c];
					if (c == ' ') {
						posx += charInfo->width;
						continue;
					}

					const float bw = charInfo->x + charInfo->width;
					const float bh = charInfo->y + charInfo->height;

					const float u0 = charInfo->x / (float)font->getWidth();
					const float v1 = charInfo->y / (float)font->getHeight();
					const float u1 = bw / (float)font->getWidth();
					const float v0 = bh / (float)font->getHeight();

					const float x = posx + charInfo->xoffset * scale.x;
					const float y = posy + charInfo->yoffset * scale.y;
					const float z = element.second->position.z;
					const float w = charInfo->width * scale.x;
					const float h = charInfo->height * scale.y;

					vertices[idx].pos = { x,		y,		z }; vertices[idx].uv = { u0, v1 }; vertices[idx].color = element.second->color;
					vertices[idx + 1].pos = { x,		y + h,	z }; vertices[idx + 1].uv = { u0, v0 }; vertices[idx + 1].color = element.second->color;
					vertices[idx + 2].pos = { x + w,	y + h,	z }; vertices[idx + 2].uv = { u1, v0 }; vertices[idx + 2].color = element.second->color;
					vertices[idx + 3].pos = { x + w,	y + h,	z }; vertices[idx + 3].uv = { u1, v0 }; vertices[idx + 3].color = element.second->color;
					vertices[idx + 4].pos = { x + w,	y,		z }; vertices[idx + 4].uv = { u1, v1 }; vertices[idx + 4].color = element.second->color;
					vertices[idx + 5].pos = { x,		y,		z }; vertices[idx + 5].uv = { u0, v1 }; vertices[idx + 5].color = element.second->color;

					float advance = charInfo->xadvance * scale.x;
					posx += advance;
					// @todo: reset and check on line break
					width += advance;
					idx += 6;
				}

				if (element.second->alignment == TextAlignment::Center) {
					for (uint32_t i = idxStartElement; i < idx; i++)
					{
						vertices[i].pos.x -= width / 2.0f;
						vertices[i].pos.y -= height / 2.0f;
					}
				}
			}
		}

		VK_CHECK_RESULT(renderer->device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data()));
	}

	void GameUI::draw(CommandBuffer* cb)
	{
		if (textElements.size() == 0) {
			return;
		}
		assert(font);

		glm::vec2 pushConstBlock[2];
		pushConstBlock[0] = glm::vec2(2.0f, 2.0f);
		pushConstBlock[1] = glm::vec2(-1.0f);

		cb->setViewport(0.0f, 0.0f, (float)renderer->width, (float)renderer->height, 0.0f, 1.0f);
		cb->setScissor(0, 0, (int32_t)renderer->width, (int32_t)renderer->height);
		cb->bindPipeline(renderer->getPipeline("msdf"));
		cb->updatePushConstant(renderer->getPipelineLayout("ui_text"), 0, &pushConstBlock);
		cb->bindDescriptorSets(renderer->getPipelineLayout("ui_text"), { font->descriptorSet }, 0);
		cb->bindVertexBuffer(vertexBuffer, 0);
		cb->draw(vertices.size(), 1, 0, 0);
	}

}