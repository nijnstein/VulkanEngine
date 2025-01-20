#include "defines.h"

namespace vkengine
{	
	namespace ui
	{
		void UIOverlay::init(VulkanDevice* vulkanDevice)
		{
			device = vulkanDevice; 

			// Init ImGui
			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(device->window, true);

			ImGuiStyle& style = ImGui::GetStyle();
			style.Colors[ImGuiCol_TitleBg]			= ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
			style.Colors[ImGuiCol_TitleBgActive]	= ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
			style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2f, 0.2f, 0.2f, 0.1f);

			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
			style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

			style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);

			// Dimensions
			ImGuiIO& io = ImGui::GetIO();
			io.FontGlobalScale = scale;
			io.DisplaySize = ImVec2(device->swapChainExtent.width, device->swapChainExtent.height);
			io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		}

		void UIOverlay::destroy() 
		{
			if (vtxDst)
			{
				delete[] vtxDst;
				vtxDst = nullptr;
			}
			if (idxDst)
			{
				delete[] idxDst;
				idxDst = nullptr;
			}

			if (ImGui::GetCurrentContext()) 
			{
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
			}
		}

		/** Prepare all vulkan resources required to render the UI overlay */
		void UIOverlay::prepareResources()
		{
			ImGuiIO& io = ImGui::GetIO();

			// Create font texture
			unsigned char* fontData;
			int texWidth, texHeight;

			const std::string filename = "assets/Roboto-Medium.ttf";
			io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);

			io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
			VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

			//SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
			ImGuiStyle& style = ImGui::GetStyle();
			style.ScaleAllSizes(scale);

			// Create target image for copy
			fontImage =	device->createImage(texWidth, texHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, fontData, uploadSize);

			// Image view
			device->createImageView(fontImage, VK_IMAGE_ASPECT_COLOR_BIT); 

			// Font texture Sampler
			VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo();
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK(vkCreateSampler(device->device, &samplerInfo, nullptr, &fontImage.sampler));
			
			// Descriptor pool
			std::vector<VkDescriptorPoolSize> poolSizes = {
				init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
			};
			VkDescriptorPoolCreateInfo descriptorPoolInfo = init::descriptorPoolCreateInfo(poolSizes, 2);
			VK_CHECK(vkCreateDescriptorPool(device->device, &descriptorPoolInfo, nullptr, &descriptorPool));

			// Descriptor set layout
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			};
			VkDescriptorSetLayoutCreateInfo descriptorLayout = init::descriptorSetLayoutCreateInfo(setLayoutBindings);
			VK_CHECK(vkCreateDescriptorSetLayout(device->device, &descriptorLayout, nullptr, &descriptorSetLayout));

			// Descriptor set
			VkDescriptorSetAllocateInfo allocInfo = init::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
			VK_CHECK(vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet));
			VkDescriptorImageInfo fontDescriptor = init::descriptorImageInfo(
				fontImage.sampler,
				fontImage.view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				init::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)
			};
			vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// shaders 
			shaders.resize(2); 
			shaders[0] = device->initVertexShader("compiled shaders/ui.vert.spv");
			shaders[1] = device->initFragmentShader("compiled shaders/ui.frag.spv");

			// buffers
			vtxDst = new ImDrawVert[UINT16_MAX];
			idxDst = new ImDrawIdx[UINT16_MAX];
		}

		/** Prepare a separate pipeline for the UI overlay rendering decoupled from the main application */
		void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat)
		{
			// Pipeline layout
			// Push constants for UI rendering parameters
			VkPushConstantRange pushConstantRange = init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = init::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			VK_CHECK(vkCreatePipelineLayout(device->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

			// Setup graphics pipeline for UI rendering
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
				init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

			VkPipelineRasterizationStateCreateInfo rasterizationState =
				init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

			// Enable blending
			VkPipelineColorBlendAttachmentState blendAttachmentState{};
			blendAttachmentState.blendEnable = VK_TRUE;
			blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlendState = init::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
            VkPipelineDepthStencilStateCreateInfo depthStencilState = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
			VkPipelineViewportStateCreateInfo viewportState = init::pipelineViewportStateCreateInfo(1, 1, 0);
			VkPipelineMultisampleStateCreateInfo multisampleState =	init::pipelineMultisampleStateCreateInfo(rasterizationSamples);
			multisampleState.sampleShadingEnable = VK_TRUE;
			multisampleState.minSampleShading = .2f;
			multisampleState.rasterizationSamples = device->getCurrentMSAASamples();

			std::vector<VkDynamicState> dynamicStateEnables = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamicState = init::pipelineDynamicStateCreateInfo(dynamicStateEnables);
			VkGraphicsPipelineCreateInfo pipelineCreateInfo = init::pipelineCreateInfo(pipelineLayout, renderPass);

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = shaders[0].module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = shaders[1].module;
			fragShaderStageInfo.pName = "main";

			std::array< VkPipelineShaderStageCreateInfo, 2> shaderCI;
			shaderCI[0] = vertShaderStageInfo;
			shaderCI[1] = fragShaderStageInfo; 

			pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
			pipelineCreateInfo.pRasterizationState = &rasterizationState;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
			pipelineCreateInfo.pMultisampleState = &multisampleState;
			pipelineCreateInfo.pViewportState = &viewportState;
			pipelineCreateInfo.pDepthStencilState = &depthStencilState;
			pipelineCreateInfo.pDynamicState = &dynamicState;
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderCI.size());
			pipelineCreateInfo.pStages = shaderCI.data();
			pipelineCreateInfo.subpass = subpass;

#if defined(VK_KHR_dynamic_rendering)
			// SRS - if we are using dynamic rendering (i.e. renderPass null), must define color, depth and stencil attachments at pipeline create time
			VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
			if (renderPass == VK_NULL_HANDLE) {
				pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
				pipelineRenderingCreateInfo.colorAttachmentCount = 1;
				pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
				pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
				pipelineRenderingCreateInfo.stencilAttachmentFormat = depthFormat;
				pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
			}
#endif

			// Vertex bindings an attributes based on ImGui vertex definition
			std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
				init::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
			};
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
				init::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	// Location 0: Position
				init::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	// Location 1: UV
				init::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),	// Location 0: Color
			};
			VkPipelineVertexInputStateCreateInfo vertexInputState = init::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
			vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			pipelineCreateInfo.pVertexInputState = &vertexInputState;

			VK_CHECK(vkCreateGraphicsPipelines(device->device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		}

		/** Update vertex and index buffer containing the imGui elements when required */
		bool UIOverlay::update()
		{
			ImDrawData* imDrawData = ImGui::GetDrawData();
			bool update = false;

			if (!imDrawData || imDrawData->TotalIdxCount == 0) 
			{ 
				return false; 
			};

			// Note: Alignment is done inside buffer creation
			vertexCount = imDrawData->TotalVtxCount;
			indexCount = imDrawData->TotalIdxCount;

			VkDeviceSize vertexBufferSize = vertexCount * sizeof(ImDrawVert);
			VkDeviceSize indexBufferSize = indexCount * sizeof(ImDrawIdx);

			// prepare data
			ImDrawVert* pVertices = vtxDst;
			ImDrawIdx* pIndices = idxDst;

			for (int n = 0; n < imDrawData->CmdListsCount; n++) 
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[n];
				memcpy(pVertices, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(pIndices, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				pVertices += cmd_list->VtxBuffer.Size;
				pIndices += cmd_list->IdxBuffer.Size;
			}

			// Vertex buffer
			if (uiVertexBuffer.buffer == VK_NULL_HANDLE) 
			{
				device->createBuffer(
					UINT16_MAX * 8,
					vtxDst,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					uiVertexBuffer,
					"vertexBuffer-UI");
				
				update = true; 
			}
			else
			{
				memcpy(uiVertexBuffer.mappedData, vtxDst, vertexBufferSize);
			}

			// Index buffer
			if ((uiIndexBuffer.buffer == VK_NULL_HANDLE)) //|| (indexCount < imDrawData->TotalIdxCount)) 
			{
				device->createBuffer(
					UINT16_MAX * 8,
					idxDst,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					uiIndexBuffer,
					"indexBuffer-UI");

				update = true;
			}
			else
			{
				memcpy(uiIndexBuffer.mappedData, idxDst, indexBufferSize);
			}

			return update;
		}

		void UIOverlay::draw(const VkCommandBuffer commandBuffer)
		{
			ImDrawData* imDrawData = ImGui::GetDrawData();
			int32_t vertexOffset = 0;
			int32_t indexOffset = 0;

			if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) 
			{
				return;
			}

			ImGuiIO& io = ImGui::GetIO();

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
			pushConstBlock.translate = glm::vec2(-1.0f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &uiVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, uiIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}

		void UIOverlay::resize(IVEC2 extent)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2((float)(extent.x), (float)(extent.y));
		}

		void UIOverlay::freeResources()
		{
			DESTROY_BUFFER(device->allocator, uiVertexBuffer)
			DESTROY_BUFFER(device->allocator, uiIndexBuffer)
			DESTROY_IMAGE(device->allocator, fontImage)

			vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
			vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
			vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
			vkDestroyPipeline(device->device, pipeline, nullptr);
		}

		bool UIOverlay::header(const char* caption)
		{
			return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
		}

		bool UIOverlay::checkBox(const char* caption, bool* value)
		{
			bool res = ImGui::Checkbox(caption, value);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::checkBox(const char* caption, int32_t* value)
		{
			bool val = (*value == 1);
			bool res = ImGui::Checkbox(caption, &val);
			*value = val;
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::radioButton(const char* caption, bool value)
		{
			bool res = ImGui::RadioButton(caption, value);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::inputFloat(const char* caption, float* value, float step, const char* format)
		{
			bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, format);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
		{
			bool res = ImGui::SliderFloat(caption, value, min, max);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
		{
			bool res = ImGui::SliderInt(caption, value, min, max);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
		{
			if (items.empty()) {
				return false;
			}
			std::vector<const char*> charitems;
			charitems.reserve(items.size());
			for (size_t i = 0; i < items.size(); i++) {
				charitems.push_back(items[i].c_str());
			}
			uint32_t itemCount = static_cast<uint32_t>(charitems.size());
			bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::button(const char* caption)
		{
			bool res = ImGui::Button(caption);
			if (res) { updated = true; };
			return res;
		}

		bool UIOverlay::colorPicker(const char* caption, float* color) {
			bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
			if (res) { updated = true; };
			return res;
		}

		void UIOverlay::text(const char* formatstr, ...)
		{
			va_list args;
			va_start(args, formatstr);
			ImGui::TextV(formatstr, args);
			va_end(args);
		}
	}
}