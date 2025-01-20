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
	
	//#
	//# Pipelines
	//# 
	void VulkanEngine::initPipelines(RenderSet& set)
	{
		for (auto materialId : set.usedMaterialIds)
		{
			auto material = materials[materialId];
			auto pipelineInfo = initGraphicsPipeline(renderPass, material);
			set.pipelines[materialId] = pipelineInfo;
		}
	}
	void VulkanEngine::recreatePipelines(RenderSet& set)
	{
		for (auto& [materialId, pi] : set.pipelines)
		{
			// TODO DESTROY PIPELINE>.... 
		}

		set.pipelines.clear(); 
		initPipelines(set); 

		set.isInvalidated = true;
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
	void VulkanEngine::drawText(std::string text, float x, float y, TextAlign align)
	{
		textCommands.push_back({ text, x, y, align });
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

			debugGrid.update(cameraController.getProjectionMatrix(), cameraController.getViewMatrix(), MAT4(1.0f), pos);
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
		return {// id                sync          size                             sparse 
			{ct_position,			true,		sizeof(VEC4),						false},		// pos 
			{ct_rotation,			true,		sizeof(QUAT),						false},		// rot
			{ct_scale,				true,		sizeof(VEC4),						false},		// scale
			{ct_color,				true,		sizeof(VEC4),						false},		// color
			{ct_boundingBox,		false,		sizeof(BBOX),                       false},		// bbox
			{ct_linearVelocity,		false,		sizeof(VEC4),						false},		// linear V 
			{ct_radialVelocity,		false,		sizeof(VEC4),						false},		// radial V 
			{ct_chunk,				false,		sizeof(Chunk), 						true},		// chunks
			{ct_chunk_id,    		false,		sizeof(UINT),						false},		// chunk id 
			{ct_mesh_id,    		false,		sizeof(MeshId),					    false},		// mesh  
			{ct_material_id,    	false,		sizeof(MaterialId),					false}		// material
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