#include "defines.h"


namespace vkengine 
{
	RenderSet* VulkanEngine::getRenderSet() 
	{ 
		return &renderSet; 
	}
	float VulkanEngine::getFrameTime()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto diff = (currentTime - startTime);
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(diff).count();
		startTime = currentTime;
		return deltaTime;
	}
	void VulkanEngine::invalidate()
	{
		invalidateComponents(ALL_COMPONENTS);
	}
	void VulkanEngine::initInputManager()
	{
		inputManager.init(this->window);
	}

	//#
	//# Pipelines
	//# 
	void VulkanEngine::initPipelines(RenderSet& set)
	{
		for (auto materialId : set.usedMaterialIds)
		{
			assert(materialId >= 0); 
			auto material = materials[materialId];
			auto pipelineInfo = initGraphicsPipeline(renderPass, material);
			set.pipelines[materialId] = pipelineInfo;
		}
	}
	void VulkanEngine::recreatePipelines(RenderSet& set)
	{
		if (!set.isPrepared)
		{
			prepareRenderSet(set); 
		}

		for (auto& [materialId, pi] : set.pipelines)
		{
			// TODO DESTROY PIPELINE>.... 
		}

		set.pipelines.clear(); 
		initPipelines(set); 

		set.isInvalidated = true;
	}
	VkDescriptorSetLayout VulkanEngine::initDescriptorSetLayout(Material material)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		// sceneInfo 
		bindings.push_back(VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
			});

		// entity data 
		for (auto& cbuffer : gpuBuffers.componentBuffers)
		{
			if (cbuffer.syncToGPU)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding = (uint32_t)bindings.size(),
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
					.pImmutableSamplers = nullptr,
					});
			}
		}

		material.getBindings(bindings);

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

		VkDescriptorSetLayout descriptorSetLayout{};
		VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));

		return descriptorSetLayout;
	}
	PipelineInfo VulkanEngine::initGraphicsPipeline(RenderPass renderPass, Material material)
	{
		PipelineInfo info{};
		info.materialId = material.materialId;
		info.descriptorSetLayout = initDescriptorSetLayout(material);
	 	 
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		if (configuration.enableWireframe)
		{
			vertShaderModule = initVertexShader(WIREFRAME_VERT_SHADER).module;
			fragShaderModule = initFragmentShader(WIREFRAME_FRAG_SHADER).module;
		}
		else
		{
			vertShaderModule = shaders[material.vertexShaderId].module;
			fragShaderModule = shaders[material.fragmentShaderId].module;
		}

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = QUANTIZED_VERTEX::getBindingDescription();
		auto attributeDescriptions = QUANTIZED_VERTEX::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;

		if (material.topology == MaterialTopology::LineList || configuration.enableWireframe)
		{
			rasterizer.cullMode = VK_CULL_MODE_NONE;
			rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
			rasterizer.lineWidth = material.topology == MaterialTopology::LineList ? material.lineWidth : 1.0f;
		}
		else
		{
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
     		rasterizer.lineWidth = 1.0f;
 		}

		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.minSampleShading = .2f;
		multisampling.rasterizationSamples = getCurrentMSAASamples();

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &info.descriptorSetLayout;

		VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &info.pipelineLayout));

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // 
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pDepthStencilState = &depthStencil;

		pipelineInfo.layout = info.pipelineLayout;
		pipelineInfo.renderPass = renderPass.getRenderPass();
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &info.pipeline));

		// create descriptors 
		for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
		{
			std::vector<VkWriteDescriptorSet> descriptorSets;

			// Scene matrices
			descriptorSets.push_back({
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = 0,
				.dstBinding = (uint32_t)descriptorSets.size(),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &sceneInfoBuffers[frame].descriptor
				});

			// entity data 
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.syncToGPU)
				{
					descriptorSets.push_back({
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = 0,
						.dstBinding = (uint32_t)descriptorSets.size(),
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pBufferInfo = &cbuffer.buffers[frame].descriptor
						});
				}
			}

			// material information 
			material.getDescriptors(this, descriptorSets);

			info.descriptors.push_back(descriptorSets);
		}

		// indirect draw buffers (if enabled) 
		if (configuration.enableIndirect)
		{
			initIndirectCommandBuffers(info);
		}

		return info;
	}

	//#
	//# Text overlay 
	//#
	void VulkanEngine::initTextOverlay()
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaders =
		{
			initVertexShader("compiled shaders/text.vert.spv").createInfo(),
			initFragmentShader("compiled shaders/text.frag.spv").createInfo()
		};

		textOverlay = new TextOverlay(
			this,
			graphicsQueue,
			renderPass.getRenderPass(),
			swapChainExtent.width, 
			swapChainExtent.height,
			configuration.textScale,
			shaders);

		updateTextOverlay(); 
	}
	bool VulkanEngine::updateTextOverlay()
	{
		if (configuration.enableTextOverlay && textOverlay)
		{
			uint32_t lastNumLetters = textOverlay->numLetters;

			if (textCommands.size() > 0)
			{
				float scale = configuration.textScale;
									
				textOverlay->beginTextUpdate();
				for (auto& cmd : textCommands)
				{
					textOverlay->addText(cmd);
				}
				textOverlay->endTextUpdate();

				textCommands.clear(); 
			}

			// If the no. of letters changed, the no. of draw commands also changes which requires a rebuild of the command buffers
			if (lastNumLetters != textOverlay->numLetters)
			{
				// must rebuild buffers 
				return true; 
			}
		}
		return false; 
	}
	void VulkanEngine::drawTextOverlay(VkCommandBuffer commandBuffer)
	{
		if (configuration.enableTextOverlay && textOverlay)
		{
			int drawCalls = textOverlay->draw(commandBuffer); 			
			updateFrameStatsDrawCount(drawCalls); 
		}
	}
	void VulkanEngine::drawText(std::string text, float x, float y, VEC3 color, float scale, TextAlign align)
	{
		textCommands.push_back({ text, x, y, 0, align, TextSpace::screenSpace, color, scale });
	}
	void VulkanEngine::drawText(std::string text, float x, float y, float z, VEC3 color, float scale, TextAlign align)
	{
		MAT4 v = cameraController.getViewMatrix(); 
		MAT4 p = cameraController.getProjectionMatrix(); 
					
		VEC4 pos = v * VEC4(x, y, z, 1);

		float xx = pos.x; // (pos.x + 1) * (swapChainExtent.width / 2.0f);
		float yy = pos.y; //-(pos.y + 1) * (swapChainExtent.height / 2.0f);

		if (xx > 0 && yy > 0 && xx < swapChainExtent.width && yy < swapChainExtent.height)
		{
			textCommands.push_back({ text, xx, yy, 0, align, TextSpace::worldSpace, color, scale });
		}
	}
	void VulkanEngine::destroyTextOverlay()
	{
		if (textOverlay)
		{
			delete textOverlay;
			textOverlay = nullptr;
		}
	}

	//#
	//# GRID
	//#
	void VulkanEngine::initGrid(RenderPass& renderPass)
	{
		debugGrid.init(this, renderPass); 
	}
	void VulkanEngine::updateGrid()
	{
		if (configuration.enableGrid)
		{
			VEC3 campos = cameraController.getPosition(); 
			VEC3 pos = { 0, campos.y, 0 };

			debugGrid.update(cameraController.getProjectionMatrix(), cameraController.getViewMatrix(), MAT4(1.0f), { 0, 0, 0 });
		}
	}
	void VulkanEngine::drawGrid(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
	{
		if (configuration.enableGrid)
		{
			debugGrid.draw(this, commandBuffer, currentFrame);
		}
	}
	void VulkanEngine::destroyGrid()
	{
		debugGrid.destroy(); 
	}

	//#
	//# UI Overlay / Window Manager 
	//#
	void VulkanEngine::initUI()
	{
		if (uiSettings.enableUI)
		{
			uiOverlay.init(this);
			uiOverlay.resize({ swapChainExtent.width, swapChainExtent.height });
			uiOverlay.prepareResources();
			uiOverlay.preparePipeline(pipelineCache, renderPass.getRenderPass(), swapChainImageFormat, findDepthFormat(physicalDevice));
		}
	}
	void VulkanEngine::updateUI(float deltaTime)
	{
		static float minTime = 0;
		static float maxTime = 1;

		if (deltaTime > 0 && uiSettings.enableUI)
		{
			// update min/max times so we dont miss them when limiting update fps
			if (minTime == 0)
			{
				minTime = maxTime = deltaTime;
			}
			else
			{
				minTime = fmin(minTime, deltaTime);
				maxTime = fmax(maxTime, deltaTime);
			}

			// check if update is needed to maintain 20fps on the ui
			uiOverlay.updateTimer -= deltaTime;
			if (uiOverlay.updateTimer >= 0) return;

			uiOverlay.updateTimer = 1.0f / 20.0f;

			ImGuiIO& io = ImGui::GetIO();
			io.DeltaTime = deltaTime;

			// update frame times 
			std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());

			uiSettings.frameTimes.back() = maxTime;

			if (minTime < uiSettings.frameTimeMin)
			{
				uiSettings.frameTimeMin = minTime;
			}

			if (maxTime > uiSettings.frameTimeMax)
			{
				uiSettings.frameTimeMax = maxTime;
			}
			minTime = maxTime = 0;

			// update ui components callback 
			ImGui::NewFrame();	 

			updateUIFrame();

			for (auto& [id, window] : windows)
			{
				if (window->isVisible())
				{
					if (frameStats.frameCounter == 1)
					{
						window->resize(this); 
					}
					window->render(this, deltaTime);
				}
			}
			ImGui::Render();

			// update vertex data
			uiOverlay.update();
		}
	}
	void VulkanEngine::drawUI(VkCommandBuffer commandBuffer)
	{
		if (uiSettings.enableUI)
		{
			uiOverlay.draw(commandBuffer);
		}
	}
	void VulkanEngine::destroyUI()
	{
		if (uiSettings.enableUI)
		{
			uiOverlay.destroy();
		}
	}

	WINDOW_ID VulkanEngine::addWindow(WINDOW* window)
	{
		windows[window->getWindowId()] = window;
		return window->getWindowId(); 
	}
	void VulkanEngine::destroyWindow(WINDOW_ID id)
	{
		if (windows.contains(id))
		{
			delete windows[id]; 
			windows.erase(id);
		}
	}
	SIZE VulkanEngine::getWindowCount()
	{
		return windows.size(); 
	}
	WINDOW* VulkanEngine::getWindow(WINDOW_ID id)
	{
		return windows[id];
	}
	WINDOW_ID VulkanEngine::hideWindow(WINDOW_ID id)
	{
		if (windows.contains(id))
		{
			windows[id]->hide(); 
		}
		return id; 
	}
	WINDOW_ID VulkanEngine::showWindow(WINDOW_ID id)
	{
		if (windows.contains(id))
		{
			windows[id]->show();
		}
		return id;
	}

	//#
	//# Entities
	//#
	std::vector<EntityComponentInfo> VulkanEngine::initEntityComponents()
	{
		setUserData(this); 

		return {// id                sync          size                             sparse 			istag 
			{ct_position,			true,		sizeof(VEC4),						false,			false},		// pos 
			{ct_rotation,			true,		sizeof(QUAT),						false,			false},		// rot
			{ct_scale,				true,		sizeof(VEC4),						false,			false},		// scale
			{ct_color,				true,		sizeof(VEC4),						false,			false},		// color
			
			// rendering 
			{ct_render_index,    	true,		sizeof(EntityId),					false,			false},		// draw instance id -> entity id
			{ct_boundingBox,		false,		sizeof(BBOX),                       false,			false},		// bbox
			{ct_chunk,				false,		sizeof(Chunk), 						true ,			false},		// chunks
			{ct_chunk_id,    		false,		sizeof(UINT),						false,			false},		// chunk id 
			{ct_mesh_id,    		false,		sizeof(MeshId),					    false,			false},		// mesh  
			{ct_material_id,    	false,		sizeof(MaterialId),					false,			false},		// material
			{ct_distance,           false,      sizeof(FLOAT),                      false,			false},
		
			// physics 
			{ct_mass,               false,      sizeof(FLOAT),                      false,			false},
			{ct_linear_velocity,    false,      sizeof(VEC4),                       false,			false},
			{ct_radial_velocity,    false,      sizeof(VEC4),                       false,			false},
			{ct_collider,           false,      sizeof(UINT),                       false,			false},

			// events 
			{ct_camera,             false,      0,	         						true,			true},      // a flag/tag from which to trigger invalidations 
			{ct_player,				false,		0,									true,           true}
		};
	}
	EntityId VulkanEngine::attachEntity(Entity entity, VEC3 pos, QUAT rot, VEC3 scale, VEC3 color)
	{
		auto id = createEntity(entity);
		((VEC4*)getComponentData(ct_position))[id]	= VEC4(pos, 0);
		((QUAT*)getComponentData(ct_rotation))[id]	= rot;
		((VEC4*)getComponentData(ct_scale))[id]		= VEC4(scale, 0);
		((VEC4*)getComponentData(ct_color))[id]		= VEC4(color, 0);
		invalidateComponents((ComponentTypeId)(ct_position | ct_rotation | ct_scale | ct_color));
		return id;
	}
	EntityId VulkanEngine::attachEntity(Entity entity, VEC3 pos, VEC3 eulerRad, VEC3 scale, VEC3 color)
	{
		auto id = createEntity(entity, ct_position | ct_rotation | ct_scale | ct_color);
		((VEC4*)getComponentData(ct_position))[id]	= VEC4(pos, 0);
		((QUAT*)getComponentData(ct_rotation))[id]	= QUAT(eulerRad);
		((VEC4*)getComponentData(ct_scale))[id]		= VEC4(scale, 0);
		((VEC4*)getComponentData(ct_color))[id]		= VEC4(color, 0);
		invalidateComponents((ComponentTypeId)(ct_position | ct_rotation | ct_scale | ct_color));
		return id; 
	}

	void VulkanEngine::onCreateEntity(EntityId entityId)
	{
		if (renderSet.isInitialized)
		{
			renderSet.isPrepared = false;
			renderSet.recreatePipelines = true; 
		}
	}
	void VulkanEngine::onRemoveEntity(EntityId entityId)
	{
		if (renderSet.isInitialized)
		{
			renderSet.isPrepared = false;
			renderSet.recreatePipelines = true;
		}
	}

	//#
	//# Indirect Rendering
	//# 
	void VulkanEngine::enableIndirectRendering()
	{
		if (!configuration.enableIndirect)
		{
			configuration.enableIndirect = true;

			// enable it on each pipeline (supporting) it 
			for (auto& [matid, pi] : renderSet.pipelines)
			{
				initIndirectCommandBuffers(pi);
			}
		}
	}
	void VulkanEngine::disableIndirectRendering()
	{
		if (configuration.enableIndirect = true)
		{
			configuration.enableIndirect = false;

			for (auto& [matid, pi] : renderSet.pipelines)
			{
				destroyIndirectCommandBuffers(pi);
			}
		}
	}
	void VulkanEngine::updateIndirectRenderInfo(RenderSet& renderSet, UINT frame, bool force)
	{
		if (configuration.enableIndirect)
		{
			for (auto& [materialId, pipeline] : renderSet.pipelines)
			{
				if (pipeline.indirectCommandBufferInvalidations.size() > 0)
				{
					if (force || pipeline.indirectCommandBufferInvalidations[frame])
					{
						if (pipeline.culledRenderInfo.size() > 0)
						{
							auto buffer = pipeline.indirectCommandBuffers[frame];
							SIZE size = pipeline.culledRenderInfo.size() * sizeof(RenderInfo);

							if (buffer.info.size < size)
							{
								std::runtime_error("draw command buffer size too small");
							}

							RenderInfo* drawCommands = nullptr;
							vmaMapMemory(allocator, buffer.alloc, (void**)&drawCommands);
							memcpy(drawCommands, pipeline.culledRenderInfo.data(), size);
							// memset(((BYTE*)drawCommands) + size, 0, buffer.info.size - size);
							vmaUnmapMemory(allocator, buffer.alloc);
						}

						pipeline.indirectCommandBufferInvalidations[frame] = false;
					}
				}
			}
		}
	}

	//#
	//# Rendering Modes 
	//# 
	void VulkanEngine::enableWireframeRendering()
	{
		if (!configuration.enableWireframe)
		{
			configuration.enableWireframe = true; 
			recreatePipelines(renderSet); 
		}
	}
	void VulkanEngine::disableWireframeRendering()
	{
		if (configuration.enableWireframe)
		{
			configuration.enableWireframe = false;
	 		recreatePipelines(renderSet);
		}
	}
	void VulkanEngine::enableVSync()
	{
		if (!configuration.enableVSync)
		{
			configuration.enableVSync = true;
			invalidateFrameBuffer(); 
		}
	}
	void VulkanEngine::disableVSync()
	{
		if (configuration.enableVSync)
		{
     		configuration.enableVSync = false;
			invalidateFrameBuffer();
		}
	}

	//#
	//# Shadow Mapping 
	//#






}