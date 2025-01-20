#pragma once

namespace vkengine
{		
	class VulkanEngine : public VulkanDevice, public EntityManager 
	{
	protected:

		std::vector<Buffer> sceneInfoBuffers;
		std::vector<DrawTextCommand> textCommands; 

		RenderPass renderPass; 
		RenderSet renderSet; 

		input::InputManager inputManager;

		ui::UIOverlay uiOverlay;
		TextOverlay* textOverlay{ nullptr };

		MAP<WINDOW_ID, WINDOW*> windows;

		DebugGrid debugGrid; 
		ModelData skyboxModel; 
		TextureInfo environmentCube; 


 	public:
		const ComponentTypeId renderPrototype = ct_position | ct_scale | ct_rotation | ct_mesh_id | ct_material_id;

		VulkanEngine(EngineConfiguration config = defaultEngineConfiguration)
		{
			configuration = config; 
		}
		CameraController cameraController;
		ui::UISettings uiSettings;

		// init / destroy 
		void init()
		{
			initWindow();
			initInstance();
			initDebugMessenger();
			initPhysicalDevice();
			initLogicalDevice();
			initExtensions(); 
			initVMA();
			initCommandPool();
			initPushDescriptors();
			initSwapChain(); 
			initCommandBuffers(); 
			initSyncObjects(); 
			initPipelineCache();
			initInputManager();
			initEntityManager(this, initEntityComponents(), 1024 * 64);  
			
			if (configuration.enablePBR)
			{
				skyboxModel = assets::loadObj("assets/skybox.obj", nullptr, 1, false, false, false, false, false, false);				 
				environmentCube = initTexture("assets/textures/gcanyon_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, true, true, true, true);
				initPBR(&skyboxModel, environmentCube.image); 
			}

			renderPass.init(this, "defaultRenderPass");

			initSwapChainBuffers(renderPass.getRenderPass());

			START_TIMER 
				DEBUG("Initializing scene\n")
				initUI();
				initGrid(renderPass);
				initTextOverlay(); 
				initScene();
				initCamera();
			END_TIMER("Scene init took ")
			
			renderSet = initRenderSet(renderPass);
		}

		void run()
		{
			while (!glfwWindowShouldClose(window))
			{
				glfwPollEvents();

				if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
				{
					ImGui_ImplGlfw_Sleep(10);
					continue;
				}

				processFrame(getCurrentFrame()); 	
			}

			vkDeviceWaitIdle(device);
		}

		void destroy()
		{
			cleanupScene(); 
			destroyTextOverlay(); 
			destroyGrid(); 
			destroyUI();
			destroyPipelineCache(); 
		}

		RenderSet* getRenderSet();
	
		void invalidate();

		void enableIndirectRendering(); 
		void disableIndirectRendering(); 

		void enableWireframeRendering(); 
		void disableWireframeRendering(); 
		
		void enableVSync(); 
		void disableVSync(); 

		WINDOW_ID hideWindow(WINDOW_ID id);
		WINDOW_ID showWindow(WINDOW_ID id);

		void drawText(std::string text, float x, float y, TextAlign align = TextAlign::alignLeft);
		//void drawText(std::string text, float x, float y, float persistForSeconds, TextAlign align = TextAlign::alignLeft);

	private: 
		float getFrameTime();

		// grid overlay
		void initGrid(RenderPass& renderPass);
		void updateGrid(); 
		void drawGrid(VkCommandBuffer& commandBuffer, uint32_t currentFrame);
		void destroyGrid(); 

		// ui overlay 
		void initUI();
		void updateUI(float deltaTime);
		void drawUI(VkCommandBuffer commandBuffer);
		void destroyUI();
	
		// text overlay
		void initTextOverlay();
		bool updateTextOverlay();
		void drawTextOverlay(VkCommandBuffer commandBuffer);
		void destroyTextOverlay();

	protected:

		// hooks for application 
		virtual void initScene() = 0; 
		virtual void initCamera() = 0; 
		virtual void updateFrame(SceneInfoBufferObject& sceneInfo, float deltaTime) = 0;
		virtual void updateUIFrame() {};
		virtual void handleResize(IVEC2 extent) {}; 
		virtual void cleanupScene() = 0;

		// window manager 
		WINDOW_ID addWindow(WINDOW* window); 
		void destroyWindow(WINDOW_ID id);
		SIZE getWindowCount(); 
		WINDOW* getWindow(WINDOW_ID id); 

		// apps may add more types
		virtual std::vector<EntityComponentInfo> initEntityComponents();

	    // helpers for creating entities 
		EntityId attachEntity(Entity entity, VEC3 pos, QUAT rot, VEC3 scale, VEC3 color);
		EntityId attachEntity(Entity entity, VEC3 pos, VEC3 eulerRad, VEC3 scale, VEC3 color);


		// handle resize events 
		void updateAfterResize()
		{
			IVEC2 dims = IVEC2(swapChainExtent.width, swapChainExtent.height); 

			if (uiSettings.enableUI)
			{
				uiOverlay.resize(dims);
				for (auto [id, window] : windows)
				{
					window->resize(this);
				}
			}
			if (configuration.enableTextOverlay && textOverlay)
			{
				textOverlay->resize(dims); 
			}

			cameraController.setViewport(dims);
			handleResize(dims);
		}
	   		
		// input 
		void initInputManager()
		{
			inputManager.init(); 
		}

		// pipeline
		VkDescriptorSetLayout initDescriptorSetLayout(Material material)
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

		PipelineInfo initGraphicsPipeline(RenderPass renderPass, Material material)
		{
			PipelineInfo info{}; 
			info.materialId = material.materialId; 
			info.descriptorSetLayout = initDescriptorSetLayout(material); 

			VkShaderModule vertShaderModule = shaders[material.vertexShaderId].module;
			VkShaderModule fragShaderModule = shaders[material.fragmentShaderId].module;

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
			rasterizer.polygonMode = configuration.enableWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

		void initPipelines(RenderSet& set);
		void recreatePipelines(RenderSet& set);

		void prepareRenderSet(RenderSet& set)
		{
			const FLOAT reserve = 1.5f;
			QUANTIZED_VERTEX* pVertices = nullptr;
			UINT* pIndices = nullptr;
			UINT indexOffset = 0;
			UINT vertexOffset = 0;
			UINT vsize = 0;
			UINT isize = 0;

			bool isCullingEnabled = configuration.cullingMode == CullingMode::full;

			set.vertexCount = 0;
			set.indexCount = 0;
			set.instanceCount = 0;
			set.meshInstances.clear();
			set.meshes.clear();
			set.usedMaterialIds.clear();
			set.meshOffsets.clear();

			// gather unique meshes and material ids 
			MeshId* e_mesh_ids = (MeshId*)getComponentData(ct_mesh_id);
			UINT* e_chunk_ids = (UINT*)getComponentData(ct_chunk_id);

			for (auto& entity : entities)
			{
				if (entity.components & renderPrototype)
				{
					MeshId meshId = e_mesh_ids[entity.index];

					// count vertices/indices 
					if (!set.meshInstances.contains(meshId))
					{
						auto* mesh = &meshes[meshId];

						if (!mesh->isLoaded() && !isCullingEnabled)
						{
							// if culling is disabled, load the mesh 
							mesh->requestMesh(this, mesh, mesh->userdataPtr); 
						}

						if (mesh->isLoaded())
						{
							set.vertexCount += (uint32_t)MAX(mesh->quantized.size(), mesh->vertices.size());
							set.indexCount += (uint32_t)mesh->indices.size();
							set.meshes.push_back(mesh);
						}
						else
						{
							// next entity as mesh isnt fetched
							set.meshes.push_back(mesh);
						}

						// only handle material on a new mesh as there is always only 1 material / mesh
						bool newMaterial = true;
						for (auto usedMaterialId : set.usedMaterialIds)
						{
							if (mesh->materialId == usedMaterialId)
							{
								newMaterial = false;
								break;
							}
						}
						if (newMaterial)
						{
							set.usedMaterialIds.push_back(mesh->materialId);
						}
					}

					// add instance to meshes
					set.meshInstances[meshId].push_back(entity.index);
				}
			}

			// check room needed, check if we can dispose enough to make room without increasing buffer sizes
			vsize = set.vertexCount * sizeof(QUANTIZED_VERTEX);
			isize = set.indexCount * sizeof(UINT);

			bool cleanDisposed =
				set.meshDisposals.size() > 0
				&&
				(set.vertexBuffer.isAllocated() && vsize > set.vertexBuffer.info.size)
				||
				(set.indexBuffer.isAllocated() && isize > set.indexBuffer.info.size);

			if (cleanDisposed)
			{
				START_TIMER
				//
				// multiple entities might use the mesh, if there is any entity in the frustum 
				// using this mesh then it should not get disposed
				// 
				// as this check can be expensive while the result is only used if buffers are too small
				// this is checked here instead of in the culling 
				// 
				std::sort(set.meshDisposals.begin(), set.meshDisposals.end(), sortMeshDisposalByDistanceH2L);
				UINT c = 0;

				for (auto& disposal : set.meshDisposals)
				{
					bool meshInUse = false; 
					for (auto& [m, pi] : set.pipelines)
					{
						for (auto& ri : pi.culledRenderInfo)
						{
							if (ri.meshId == disposal.meshId)
							{
								meshInUse = true;
								break;
							}
						}
					}
					if (!meshInUse)
					{
						DEBUG("entity: %d, disposing mesh data for mesh %d", disposal.entityId, disposal.meshId)

						auto& mesh = meshes[disposal.meshId];

						set.vertexCount -= MAX(mesh.quantized.size(), mesh.vertices.size());
						set.indexCount -= mesh.indices.size();

						mesh.vertices.resize(0);
						mesh.quantized.resize(0);
						mesh.indices.resize(0);

						vsize = set.vertexCount * sizeof(QUANTIZED_VERTEX);
						isize = set.indexCount * sizeof(UINT);

						cleanDisposed =
							set.meshDisposals.size() > 0
							&&
							(set.vertexBuffer.isAllocated() && vsize > set.vertexBuffer.info.size)
							||
							(set.indexBuffer.isAllocated() && isize > set.indexBuffer.info.size);

						if (!cleanDisposed)
						{
							break; 
						}
					}
				}
				if (c > 0) END_TIMER("disposed %d meshes in ", c);
			}

			// recreate v/i buffers 
			if (set.vertexCount > 0 && set.indexCount > 0)
			{
				if (set.vertexBuffer.isAllocated() && vsize > set.vertexBuffer.info.size)
				{
					DESTROY_BUFFER(allocator, set.vertexBuffer)
				}

				if (set.indexBuffer.isAllocated() && isize > set.indexBuffer.info.size)
				{
					DESTROY_BUFFER(allocator, set.indexBuffer)
				}

				if (!set.vertexBuffer.isAllocated())
				{
					createBuffer(
						MAX(vsize, configuration.minEntityVertexBufferSize),
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						// VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
						VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
						set.vertexBuffer,
						"vertexBuffer",
						reserve);
				}

				if (!set.indexBuffer.isAllocated())
				{
					createBuffer(
						MAX(isize, configuration.minEntityIndexBufferSize),
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
						// VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
						VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
						set.indexBuffer,
						"indexBuffer",
						reserve);
				}

				// write into vbuffers 
				pVertices = (QUANTIZED_VERTEX*)set.vertexBuffer.mappedData;
				pIndices = (UINT*)set.indexBuffer.mappedData;

				// set vertices and indices foreach unique mesh ordered by material
				for (auto materialId : set.usedMaterialIds)
				{
					for (auto mesh : set.meshes)
					{
						if (mesh->materialId != materialId)
						{
							continue;
						}

						if (mesh->aabb.halfSize.x == 0)
						{
							mesh->calculateAABB();
						}

						if (mesh->indices.size() > 0)
						{
							if (!mesh->isQuantized())
							{
								mesh->quantize();
							}

							UINT indexCount = (UINT)mesh->indices.size();
							UINT vertexCount = (UINT)mesh->quantized.size();

							MeshOffsetInfo offsets{};
							offsets.indexOffset = indexOffset;
							offsets.instanceOffset = set.instanceCount;
							offsets.indexCount = indexCount;
							offsets.vertexCount = vertexCount;
							offsets.vertexOffset = vertexOffset;
							offsets.meshId = mesh->meshId;
							offsets.materialId = mesh->materialId;
							offsets.bufferIndex = 0;

							// copy vertices and indices into gpu buffer 
							memcpy(pVertices, mesh->quantized.data(), vertexCount * sizeof(QUANTIZED_VERTEX));
							pVertices += vertexCount;
							vertexOffset += vertexCount;

							memcpy(pIndices, mesh->indices.data(), indexCount * sizeof(UINT));
							pIndices += indexCount;
							indexOffset += indexCount;

							// attach entities using this mesh to the offset 
							for (auto entityId : set.meshInstances[mesh->meshId])
							{
								offsets.instances.push_back(entityId);
								set.instanceCount++;
							}
							set.meshOffsets[mesh->meshId] = offsets;
						}
					}
				}
			}
			set.invalidateVertexBuffers = false;
		}

		RenderSet initRenderSet(RenderPass& renderPass)
		{
			START_TIMER

			RenderSet set{};  
			prepareRenderSet(set);

			// create buffers for shared shader info 
			sceneInfoBuffers.resize(MAX_FRAMES_IN_FLIGHT);

			char nameBuffer[64];
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				sprintf_s(nameBuffer, "sceneInfoBuffer[%d]", i); 
				createBuffer(
					sizeof(SceneInfoBufferObject),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					sceneInfoBuffers[i],
					&nameBuffer[0]);
			}

			initPipelines(set); 

			END_TIMER("Renderset:\n- vertexCount:   %d\n- indexCount:    %d\n- instanceCount: %d\n- materialCount: %d\n- meshCount:     %d\n- time:          ", set.vertexCount, set.indexCount, set.instanceCount, set.usedMaterialIds.size(), set.meshes.size())
			return set; 
		}

		void updateRenderSet(RenderSet& set, MAT4 view, MAT4 projection, float far)
		{
			// if invalidated, rebuild before the cull because of changing meshoffsets 
			if (set.invalidateVertexBuffers)
			{
				prepareRenderSet(set); 
			}

			if (configuration.cullingMode != CullingMode::disabled)
			{
				cullRenderSet(set, view, projection, far); 
			}
		}
	   				   		
		void cullRenderSet(RenderSet& set, MAT4 view, MAT4 projection, float far)
		{
			/*Here is a crazy idea: you could sort your triangles according to the direction of their normal.

			Lets say you have a vertex buffer who's corresponding direction vector is (1, 0, 0). 
			Now during initialization, you put all triangles in this buffer that have a normal that doesn't differ 
			from this buffer's direction for more than 45 degrees (arbitrary number).

			On every frame, you can now calculate the difference between the buffer vector and the camera vector.
			If the difference is large enough, you can completely skip drawing all the triangles because they 
			will be back face culled anyway. This saves the GPU from running millions of vertex shader invocations.*/

			static MAT4 lastVP = {}; 
			MAT4 vp = projection * view; 

			bool frustumChanged = (vp != lastVP);
			lastVP = vp; 

			static UINT openMeshRequests = 0; 
			bool meshRequestsReady = set.meshRequests.size() < openMeshRequests; 

			static bool firstCull = true; 

			// check if we could skip this 
			if (!(meshRequestsReady // needs to-recull to include rendering the mesh
				||
				set.isInvalidated
				||
				firstCull /* first init */ ))
			{
				if (!isEntityDataInvalidated()) {
					return;
				}
				else
					if (!frustumChanged) {
						return;
					}
			}

			resetEntityDataInvalidation();

			firstCull = false; 
			set.meshDisposals.clear(); 

			Frustum frustum = Frustum(projection * view);

			int instanceOffset = 0;
			int totalInstanceCount = 0; 
			VEC3 eye = cameraController.getPosition(); 

			std::array<std::vector<EntityId>, LOD_LEVELS> lodData;

			VEC4* pP = (VEC4*)gpuBuffers.getBuffer(getCurrentFrame(), ct_position).mappedData;
			QUAT* pR = (QUAT*)gpuBuffers.getBuffer(getCurrentFrame(), ct_rotation).mappedData;
			VEC4* pS = (VEC4*)gpuBuffers.getBuffer(getCurrentFrame(), ct_scale).mappedData;

			VEC4* positions = (VEC4*)getComponentData(ct_position);
			QUAT* rotations = (QUAT*)getComponentData(ct_rotation);
			VEC4* scales = (VEC4*)getComponentData(ct_scale);
			BBOX* bboxs = (BBOX*)getComponentData(ct_boundingBox); 
		
			set.meshRequests.clear(); 

			if (configuration.cullingMode == CullingMode::full)
			{ 
				for (auto materialId : set.usedMaterialIds)
				{
					UINT materialIdInstanceCount = 0; 
					auto& pipelineInfo = set.pipelines[materialId];
					pipelineInfo.culledRenderInfo.clear();

					for (auto mesh : set.meshes)
					{
						if (mesh->materialId != materialId)
						{
							continue; 
						}

						for (int i = 0; i < lodData.size(); i++)
						{
							lodData[i].clear();
						}

						int instanceCount = 0;
						bool haslods = false; 

						for (auto entityId : set.meshInstances[mesh->meshId])
						{
							BBOX box = bboxs[entityId];
																					
							if (frustum.IsBoxVisible(box.min, box.max))
							{
								if (!mesh->isLoaded() && mesh->requestMesh != nullptr && mesh->userdataPtr != nullptr)
								{
									// request mesh data for next frame, closest should be loaded first 									
									VEC3 center = box.Center(); 
									VEC3 pos = VEC3(center.x, eye.y, center.z);
									float distance = ABS(DISTANCE(eye, pos));

									set.meshRequests.push_back({ entityId, mesh->meshId, distance });
								}
								else
								{
									// add to render data
									UINT lodLevels = mesh->lods.size(); 
									if (configuration.enableLOD && lodLevels > 1)
									{
										haslods = true;
										VEC3 pos = mesh->aabb.center + VEC3(positions[entityId]);
										float distance = ABS(DISTANCE(eye, pos));
										UINT lodLevel = lodLevels - 1;
										for (UINT i = 0; i < lodLevels - 1; i++)
										{
											if (distance < mesh->lodDistances[i])
											{
												lodLevel = i;
												break;
											}
										}

										// collect data for each lod level as each lod needs a different draw call
										lodData[lodLevel].push_back(entityId);
									}
									else
									{
										// all the same lod if disabled (could write data directly)
										lodData[0].push_back(entityId);
									}
									instanceCount++;
								}
							}
							else
							{
								// not visible and requestable, might remove from set if distant 
								// these will be removed from the buffers if room is needed in this frame
								if (mesh->isLoaded() && mesh->requestMesh != nullptr && mesh->userdataPtr)
								{
									VEC3 center = box.Center();
									VEC3 pos = VEC3(center.x, eye.y, center.z);
									float distance = ABS(DISTANCE(eye, pos));
									if (distance > far * 0.7f)
									{
										set.meshDisposals.push_back({ entityId, mesh->meshId, distance, MAX(mesh->quantized.size(), mesh->vertices.size()), mesh->indices.size() });
									}
								}
							}
						}

						if (instanceCount > 0)
						{
							materialIdInstanceCount += instanceCount; 

							MeshOffsetInfo& offset = set.meshOffsets[mesh->meshId];

							if (haslods)
							{
								// issue 1 instanceddraw foreach lodlevel used per mesh 
								for (int lodIndex = 0; lodIndex < lodData.size(); lodIndex++)  // lod level
								{
									unsigned int size = lodData[lodIndex].size();  // instances in lod level 
									if (size > 0)
									{
										RenderInfo rinfo{};
										rinfo.meshId = mesh->meshId;
										rinfo.lodLevel = lodIndex;

										rinfo.command.firstInstance = instanceOffset;
										rinfo.command.instanceCount = size;

										rinfo.command.firstIndex = offset.indexOffset + mesh->lods[lodIndex].indexOffset;
										rinfo.command.indexCount = mesh->lods[lodIndex].indexCount;

										rinfo.command.vertexOffset = offset.vertexOffset; 
										rinfo.vertexCount = offset.vertexCount;

										pipelineInfo.culledRenderInfo.push_back(rinfo);

										instanceOffset += size;
										totalInstanceCount += size;

										// update data for render 
										// -> this updates in the wrong order ???
										auto& level = lodData[lodIndex];
										for (int j = 0; j < level.size(); j++) memcpy(pP++, &positions[level[j]], sizeof(VEC4));
										for (int j = 0; j < level.size(); j++) memcpy(pR++, &rotations[level[j]], sizeof(QUAT));
										for (int j = 0; j < level.size(); j++) memcpy(pS++, &scales[level[j]], sizeof(VEC4));
									}
								}
							}
							else
							{
								// no lods, 1 draw for all instances of this mesh
								RenderInfo rinfo{};
								rinfo.meshId = mesh->meshId;
								rinfo.lodLevel = 0; // for debugging forcing to show lod 1 

								rinfo.command.firstInstance = instanceOffset;
								rinfo.command.instanceCount = instanceCount;

								rinfo.command.firstIndex = offset.indexOffset + mesh->lods[rinfo.lodLevel].indexOffset;
								rinfo.command.indexCount = mesh->lods[rinfo.lodLevel].indexCount;
						
								rinfo.command.vertexOffset = offset.vertexOffset;
								rinfo.vertexCount = offset.vertexCount;

								pipelineInfo.culledRenderInfo.push_back(rinfo);

								instanceOffset += instanceCount;
								totalInstanceCount += instanceCount;

								// copy data from entitydata to gpu buffers 
								auto& level = lodData[0];
								for (int j = 0; j < level.size(); j++) memcpy(pP++, &positions[level[j]], sizeof(VEC4));
								for (int j = 0; j < level.size(); j++) memcpy(pR++, &rotations[level[j]], sizeof(QUAT));
								for (int j = 0; j < level.size(); j++) memcpy(pS++, &scales[level[j]], sizeof(VEC4));
							}
						}
					}

					if (materialIdInstanceCount > 0)
					{
						invalidateDrawCommandBuffers(pipelineInfo);
					}
				}
			}

			set.culledInstanceCount = totalInstanceCount; 
			set.isInvalidated = false;

			openMeshRequests = set.meshRequests.size(); 
		}

		void updateIndirectRenderInfo(RenderSet& renderSet, UINT frame, bool force = false); 

		// frame update
		void updateFrameData(RenderSet& set, uint32_t currentFrame, SceneInfoBufferObject& sceneInfo)
		{
			// scene information
			memcpy(sceneInfoBuffers[currentFrame].mappedData, &sceneInfo, sizeof(SceneInfoBufferObject));

			if (configuration.cullingMode == CullingMode::disabled)
			{
				//## copy full entity data buffer as all entities are in order and used ##
				syncDirty(currentFrame);
			}

			// handle any request for new meshes 
			handleMeshRequests(set); 
		}

		static bool sortMeshRequestByDistanceL2H(const MeshRequest a, const MeshRequest b)
		{
			return a.distance < b.distance;
		}	
		
		static bool sortMeshDisposalByDistanceH2L(const MeshDisposal a, const MeshDisposal b)
		{
			return a.distance < b.distance;
		}

		bool tryAppendMeshToSetBuffers(RenderSet& set, MeshInfo* mesh, EntityId entityId)
		{
			if (set.vertexBuffer.isAllocated())
			{
				// append data to vertex and index buffer, if it fits 
				// then we dont need to invalidate the full buffer 
				
				//*
				//* TODO : check for free space left by a meshdisposal 
				//* TODO : allocate a new buffer if needed and add it to the set 
				//*

				SIZE vsize = mesh->quantized.size() * sizeof(QUANTIZED_VERTEX);
				SIZE vtotal = set.vertexCount * sizeof(QUANTIZED_VERTEX) + vsize;

				SIZE isize = mesh->indices.size() * sizeof(UINT);
				SIZE itotal = set.indexCount * sizeof(UINT) + isize;

				bool fits = vtotal <= set.vertexBuffer.info.size && itotal <= set.indexBuffer.info.size;
				if (fits)
				{
					// copy vertices
					QUANTIZED_VERTEX* pv = (QUANTIZED_VERTEX*)set.vertexBuffer.mappedData;
					pv += set.vertexCount;
					
					QUANTIZED_VERTEX* pvm = mesh->quantized.data();
					memcpy(pv, pvm, mesh->quantized.size() * sizeof(QUANTIZED_VERTEX));
			
					// copy indices 
					UINT* pi = (UINT*)set.indexBuffer.mappedData;
					pi += set.indexCount;
					memcpy(pi, mesh->indices.data(), sizeof(UINT) * mesh->indices.size());

					// update meshoffset 
					MeshOffsetInfo offset
					{
						.meshId = mesh->meshId,
						.materialId = mesh->materialId,
						.vertexOffset = (int)set.vertexCount,
						.indexOffset = (int)set.indexCount,
						.instanceOffset = (int)set.instanceCount,
						.bufferIndex = 0
					};
					offset.instances.push_back(entityId);

					set.meshOffsets[mesh->meshId] = offset;

					set.vertexCount += mesh->quantized.size();
					set.indexCount += mesh->indices.size();
					set.instanceCount += 1;

					/* data did fit, buffers updated */
					return true;
				}
				else
				// it didnt fit
				{
					// any disposed that might fit? 
					// sort on (distance away * size) get smallest fit 

				}
			}

			return false;
		}

		void handleMeshRequests(RenderSet& set)
		{
			if (set.meshRequests.size() == 0)
			{
				return;
			}

			START_TIMER

			int requestIndex = 0; 

			std::sort(set.meshRequests.begin(), set.meshRequests.end(), sortMeshRequestByDistanceL2H);

			do
			{
				MeshRequest& req = set.meshRequests[requestIndex];

				DEBUG("entity %d, requesting mesh with id: %d at distance %.02f\n", req.entityId, req.meshId, req.distance)

				auto mesh = &meshes[req.meshId];
				mesh->requestMesh(this, mesh, mesh->userdataPtr);
				if (!mesh->isQuantized())
				{
					mesh->quantize(); 
				}

				if (!set.invalidateVertexBuffers)
				{
					if (!tryAppendMeshToSetBuffers(set, mesh, req.entityId))
					{
						set.invalidateVertexBuffers = true;
					}
				}
				
				requestIndex++;
			}
			// keep running for more meshes if we introduce a complete buffer invalidation
			while (requestIndex < 10 && requestIndex < set.meshRequests.size() && set.invalidateVertexBuffers);

			END_TIMER("requested %d meshes in ", requestIndex)
		
			set.meshRequests.clear();
		}

		void processFrame(uint32_t currentFrame)
		{	 			 
			vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

			uint32_t imageIndex;
			VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				recreateSwapChain(renderPass.getRenderPass());
				return;
			}
			else
			if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("failed to acquire swap chain image!");
			}

			float deltaTime = getFrameTime(); 
			frameStats.frameTime = deltaTime;
			frameStats.frameCounter += 1; 

		    inputManager.update(window, deltaTime);
			cameraController.update(inputManager, deltaTime); 

			SceneInfoBufferObject sceneInfo{};
			sceneInfo.viewPosition = cameraController.getPosition();
			sceneInfo.view = cameraController.getViewMatrix(); 
			sceneInfo.proj = cameraController.getProjectionMatrix(); 
			sceneInfo.projectionView = cameraController.getViewProjectionMatrix(); 
			sceneInfo.normal = glm::transpose(glm::inverse(MAT4(1)));

			updateFrame(sceneInfo, deltaTime);
			updateRenderSet(renderSet, sceneInfo.view, sceneInfo.proj, cameraController.getIntrinsic().far);
			updateFrameData(renderSet, currentFrame, sceneInfo);
			updateIndirectRenderInfo(renderSet, currentFrame); 

			updateUI(deltaTime); 	
			updateGrid();
			updateTextOverlay(); 
			
			vkResetFences(device, 1, &inFlightFences[currentFrame]); 
			vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VK_CHECK(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo))

			drawFrame(renderPass, renderSet, commandBuffers[currentFrame], imageIndex);
	
			VK_CHECK(vkEndCommandBuffer(commandBuffers[currentFrame]));

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

			VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			result = vkQueuePresentKHR(presentQueue, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || hasFrameBufferResized())
			{
				recreateSwapChain(renderPass.getRenderPass());
			}
			else
				if (result != VK_SUCCESS)
				{
					throw std::runtime_error("failed to present swap chain image!");
				}

			currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		void drawFrame(RenderPass renderPass, RenderSet& renderSet, VkCommandBuffer commandBuffer, uint32_t imageIndex)
		{
			resetFrameStats(); 
			UINT currentFrame = getCurrentFrame();

			renderPass.begin(this, commandBuffer, swapChainFramebuffers[imageIndex], swapChainExtent);

			if (renderSet.vertexBuffer.isAllocated() && renderSet.indexBuffer.isAllocated())
			{
				VkBuffer vertexBuffers[] = { renderSet.vertexBuffer.buffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, renderSet.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

				for (auto& [materialId, pipelineInfo] : renderSet.pipelines)
				{
   				    if (pipelineInfo.culledRenderInfo.size() == 0) continue;

					auto& material = materials[materialId];

					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.pipeline);

					vkCmdPushDescriptorSetKHR(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineInfo.pipelineLayout,
						0,
						(uint32_t)pipelineInfo.descriptors[currentFrame].size(),
						pipelineInfo.descriptors[currentFrame].data());

					if (configuration.enableIndirect)
					{
						vkCmdDrawIndexedIndirect(
							commandBuffer,
							pipelineInfo.indirectCommandBuffers[currentFrame].buffer,
							0,
							pipelineInfo.culledRenderInfo.size(),
							sizeof(RenderInfo));

						for (auto& ri : pipelineInfo.culledRenderInfo)
						{
							updateFrameStats(ri.command.instanceCount, ri.command.indexCount / 3, ri.lodLevel); 
						}
						updateFrameStatsDrawCount(1);
					}
					else
					if (configuration.cullingMode == CullingMode::disabled)
					{
						vkCmdDrawIndexed(
							commandBuffer,
							renderSet.indexCount,
							1,
							0,
							0,
							0);

						updateFrameStats(renderSet.instanceCount, renderSet.indexCount / 3 * renderSet.instanceCount, 0);
						updateFrameStatsDrawCount(1);
					}
					else
					{
						for (auto& renderInfo : pipelineInfo.culledRenderInfo)
						{
							vkCmdDrawIndexed(
								commandBuffer,
								renderInfo.command.indexCount,
								renderInfo.command.instanceCount,
								renderInfo.command.firstIndex,
								renderInfo.command.vertexOffset,
								renderInfo.command.firstInstance);

							updateFrameStats(renderInfo.command.instanceCount, renderInfo.command.indexCount / 3 * renderInfo.command.instanceCount, renderInfo.lodLevel);
							updateFrameStatsDrawCount(1);
						}
					}
				}
			}

			drawGrid(commandBuffer, getCurrentFrame());
			drawTextOverlay(commandBuffer);
			drawUI(commandBuffer);

			renderPass.end(commandBuffer); 
		}

	};
}





