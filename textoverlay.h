#pragma once
#include "stb_font_consolas_24_latin1.inl"

// Max. number of chars the text overlay buffer can hold
#define TEXTOVERLAY_MAX_CHAR_COUNT 2048


namespace vkengine
{
	enum TextAlign { alignLeft, alignCenter, alignRight };
	enum TextSpace { screenSpace, worldSpace };
	
	struct DrawTextCommand
	{												   
		std::string text{};
		float x{ 0 };
		float y{ 0 };
		float z{ 0 };

		TextAlign align{ TextAlign::alignLeft };
		TextSpace space{ TextSpace::screenSpace }; 

		VEC3 color{ 1, 1, 1 };
		float scale{ 1 };
	};

	/*
		Mostly self-contained text overlay class
		This class contains all Vulkan resources for drawing the text overlay
		It can be plugged into an existing renderpass/command buffer
	*/
	class TextOverlay
	{
	private:
		// Created by this class
		// Font image
		Image image;
		
		// Character vertex buffer
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
		VkPipelineLayout pipelineLayout;
		VkPipelineCache pipelineCache;
		VkPipeline pipeline;

		// Passed from the sample
		VkRenderPass renderPass;
		VkQueue queue;
		VulkanDevice* vulkanDevice; 
		uint32_t frameBufferWidth;
		uint32_t frameBufferHeight;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		float scale;

		// Pointer to mapped vertex buffer
		glm::vec4* mapped = nullptr;

		stb_fontchar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];
	public:

		uint32_t numLetters;

		TextOverlay(
			VulkanDevice* vulkanDevice,
			VkQueue queue,
			VkRenderPass renderPass,
			uint32_t framebufferwidth,
			uint32_t framebufferheight,
			float scale,
			std::vector<VkPipelineShaderStageCreateInfo> shaderstages)
		{
			this->vulkanDevice = vulkanDevice;
			this->queue = queue;
			this->shaderStages = shaderstages;
			this->frameBufferWidth = framebufferwidth;
			this->frameBufferHeight = framebufferheight;
			this->scale = scale;
			this->renderPass = renderPass;

			init(); 
		}

		~TextOverlay()
		{
			destroy(); 
		}

		void init()
		{
			prepareResources();
			preparePipeline();
		}

		void destroy()
		{
			DESTROY_IMAGE(vulkanDevice->allocator, image)

			vkDestroyBuffer(vulkanDevice->device, buffer, nullptr);
			vkFreeMemory(vulkanDevice->device, memory, nullptr);
			vkDestroyDescriptorSetLayout(vulkanDevice->device, descriptorSetLayout, nullptr);
			vkDestroyDescriptorPool(vulkanDevice->device, descriptorPool, nullptr);
			vkDestroyPipelineLayout(vulkanDevice->device, pipelineLayout, nullptr);
			vkDestroyPipelineCache(vulkanDevice->device, pipelineCache, nullptr);
			vkDestroyPipeline(vulkanDevice->device, pipeline, nullptr);
		}

		void resize(VEC2 dims)
		{
			destroy();
			frameBufferWidth = dims.x;
			frameBufferHeight = dims.y;
			init(); 
		}

		// Prepare all vulkan resources required to render the font
		// The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
		void prepareResources()
		{
			const UINT fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
			const UINT fontHeight = STB_FONT_consolas_24_latin1_BITMAP_HEIGHT;

			static unsigned char font24pixels[fontHeight][fontWidth];
			stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

			// Vertex buffer
			VkDeviceSize bufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * sizeof(VEC4) * 2;

			VkBufferCreateInfo bufferInfo = init::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
			VK_CHECK(vkCreateBuffer(vulkanDevice->device, &bufferInfo, nullptr, &buffer));

			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo allocInfo = init::memoryAllocateInfo();

			vkGetBufferMemoryRequirements(vulkanDevice->device, buffer, &memReqs);
			allocInfo.allocationSize = memReqs.size;
			allocInfo.memoryTypeIndex = findMemoryType(vulkanDevice->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VK_CHECK(vkAllocateMemory(vulkanDevice->device, &allocInfo, nullptr, &memory));
			VK_CHECK(vkBindBufferMemory(vulkanDevice->device, buffer, memory, 0));

			// Font texture
			image = vulkanDevice->createImage(
				fontWidth, fontHeight, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_FORMAT_R8_UNORM,
				VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				&font24pixels[0][0],
				fontWidth * fontHeight);

			vulkanDevice->createImageView(image); 

			VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo();
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			vulkanDevice->createImageSampler(image, samplerInfo);

			// Descriptor
			// Font uses a separate descriptor pool
			std::array<VkDescriptorPoolSize, 1> poolSizes;
			poolSizes[0] = init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

			VkDescriptorPoolCreateInfo descriptorPoolInfo = init::descriptorPoolCreateInfo(
					static_cast<uint32_t>(poolSizes.size()),
					poolSizes.data(),
					1);

			VK_CHECK(vkCreateDescriptorPool(vulkanDevice->device, &descriptorPoolInfo, nullptr, &descriptorPool));

			// Descriptor set layout
			std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings;
			setLayoutBindings[0] = init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = init::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
			VK_CHECK(vkCreateDescriptorSetLayout(vulkanDevice->device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

			// Descriptor set
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo = init::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
			VK_CHECK(vkAllocateDescriptorSets(vulkanDevice->device, &descriptorSetAllocInfo, &descriptorSet));

			// Descriptor for the font image
			VkDescriptorImageInfo texDescriptor = init::descriptorImageInfo(image.sampler, image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
			writeDescriptorSets[0] = init::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptor);
			vkUpdateDescriptorSets(vulkanDevice->device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
		}

		// Prepare a separate pipeline for the font rendering decoupled from the main application
		void preparePipeline()
		{
			// Pipeline cache
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			VK_CHECK(vkCreatePipelineCache(vulkanDevice->device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

			// Layout
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = init::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
			VK_CHECK(vkCreatePipelineLayout(vulkanDevice->device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

			// Enable blending, using alpha from red channel of the font texture (see text.frag)
			VkPipelineColorBlendAttachmentState blendAttachmentState{};
			blendAttachmentState.blendEnable = VK_TRUE;
			blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, VK_FALSE);
			VkPipelineRasterizationStateCreateInfo rasterizationState = init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
			VkPipelineColorBlendStateCreateInfo colorBlendState       = init::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
			VkPipelineDepthStencilStateCreateInfo depthStencilState   = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
			VkPipelineViewportStateCreateInfo viewportState           = init::pipelineViewportStateCreateInfo(1, 1, 0);
			VkPipelineMultisampleStateCreateInfo multisampleState     = init::pipelineMultisampleStateCreateInfo(vulkanDevice->getCurrentMSAASamples(), 0);
			std::vector<VkDynamicState> dynamicStateEnables           = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicState             = init::pipelineDynamicStateCreateInfo(dynamicStateEnables);

			std::array<VkVertexInputBindingDescription, 2> vertexInputBindings = {
				init::vertexInputBindingDescription(0, sizeof(VEC4) * 2, VK_VERTEX_INPUT_RATE_VERTEX),
				init::vertexInputBindingDescription(1, sizeof(VEC4), VK_VERTEX_INPUT_RATE_VERTEX),
			};
			std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes = {
				init::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0),			// Location 0: Position
				init::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(VEC2)),	// Location 1: UV
				init::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(VEC4)),	// Location 2: Color + 1 value free 
			};

			VkPipelineVertexInputStateCreateInfo vertexInputState = init::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
			vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			VkGraphicsPipelineCreateInfo pipelineCreateInfo = init::pipelineCreateInfo(pipelineLayout, renderPass, 0);
			pipelineCreateInfo.pVertexInputState = &vertexInputState;
			pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
			pipelineCreateInfo.pRasterizationState = &rasterizationState;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
			pipelineCreateInfo.pMultisampleState = &multisampleState;
			pipelineCreateInfo.pViewportState = &viewportState;
			pipelineCreateInfo.pDepthStencilState = &depthStencilState;
			pipelineCreateInfo.pDynamicState = &dynamicState;
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCreateInfo.pStages = shaderStages.data();

			VK_CHECK(vkCreateGraphicsPipelines(vulkanDevice->device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		}

		// Map buffer
		void beginTextUpdate()
		{
			VK_CHECK(vkMapMemory(vulkanDevice->device, memory, 0, VK_WHOLE_SIZE, 0, (void**)&mapped));
			numLetters = 0;
		}

		void addText(DrawTextCommand& cmd)
		{
			switch (cmd.space)
			{
			case TextSpace::screenSpace:
				addText(cmd.text, cmd.x, cmd.y, cmd.color, cmd.scale, cmd.align);
				break;
			
			case TextSpace::worldSpace:
				// pre-transformed... 
				addText(cmd.text, cmd.x, cmd.y, cmd.color, cmd.scale, cmd.align);
				break;
			}
		}

		float calculateWidth(std::string text, float textScale)
		{
			const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;
			assert(mapped != nullptr);

			const float charW = textScale * 1.5f * scale / frameBufferWidth;
			const float charH = textScale * 1.5f * scale / frameBufferHeight;
							 			
			float textWidth = 0;
			float textHeight = 0; 
			for (auto letter : text)
			{
				stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
				textWidth += charData->advance * charW;
			}

			return textWidth;
		}

		// Add text to the current buffer
		void addText(std::string text, float x, float y, VEC3 color, float textScale, TextAlign align)
		{
			const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;
			assert(mapped != nullptr);

			const float charW = textScale * 1.5f * scale / frameBufferWidth;
			const float charH = textScale * 1.5f * scale / frameBufferHeight;

			float fbW = (float)frameBufferWidth;
			float fbH = (float)frameBufferHeight;
			x = (x / fbW * 2.0f) - 1.0f;
			y = (y / fbH * 2.0f) - 1.0f;

			// Calculate text width
			float textWidth = 0;
			for (auto letter : text)
			{
				stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
				textWidth += charData->advance * charW;
			}

			switch (align)
			{
			case alignRight:
				x -= textWidth;
				break;
			case alignCenter:
				x -= textWidth / 2.0f;
				break;
			case alignLeft:
				break;
			}

			// Generate a uv mapped quad per char in the new text
			for (auto letter : text)
			{
				stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];

				mapped->x = (x + (float)charData->x0 * charW);
				mapped->y = (y + (float)charData->y0 * charH);
				mapped->z = charData->s0;
				mapped->w = charData->t0;
				mapped++;
				mapped->x = color.r;
				mapped->y = color.g;
				mapped->z = color.b;
				mapped->w = 1;
				mapped++;

				mapped->x = (x + (float)charData->x1 * charW);
				mapped->y = (y + (float)charData->y0 * charH);
				mapped->z = charData->s1;
				mapped->w = charData->t0;
				mapped++;
				mapped->x = color.x;
				mapped->y = color.y;
				mapped->z = color.z;
				mapped->w = 1;
				mapped++;

				mapped->x = (x + (float)charData->x0 * charW);
				mapped->y = (y + (float)charData->y1 * charH);
				mapped->z = charData->s0;
				mapped->w = charData->t1;
				mapped++;
				mapped->x = color.r;
				mapped->y = color.g;
				mapped->z = color.b;
				mapped->w = 1;
				mapped++;

				mapped->x = (x + (float)charData->x1 * charW);
				mapped->y = (y + (float)charData->y1 * charH);
				mapped->z = charData->s1;
				mapped->w = charData->t1;
				mapped++;
				mapped->x = color.r;
				mapped->y = color.g;
				mapped->z = color.b;
				mapped->w = 1;
				mapped++;

				x += charData->advance * charW;

				numLetters++;
			}
		}

		void endTextUpdate()
		{
			vkUnmapMemory(vulkanDevice->device, memory);
			mapped = nullptr;
		}

		// Issue the draw commands for the characters of the overlay
		int draw(VkCommandBuffer commandBuffer)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offsets);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &buffer, &offsets);
			
			// One draw command for every character. This is okay for a debug overlay, but not optimal
			// In a real-world application one would try to batch draw commands
			for (uint32_t j = 0; j < numLetters; j++) 
			{
				vkCmdDraw(commandBuffer, 4, 1, j * 4, 0);
			}

			return numLetters; 
		}
	};
}