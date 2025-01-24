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

		ui::UIOverlay uiOverlay;
		TextOverlay* textOverlay{ nullptr };

		MAP<WINDOW_ID, WINDOW*> windows;

		DebugGrid debugGrid; 
		ModelData skyboxModel; 
		TextureInfo environmentCube; 


 	public:
		const ComponentTypeId renderPrototype = ct_position | ct_scale | ct_rotation | ct_mesh_id | ct_material_id | ct_boundingBox | ct_render_index | ct_distance;
		const ComponentTypeId invisiblePrototype = ct_position | ct_scale | ct_rotation | ct_material_id | ct_boundingBox;

		VulkanEngine(EngineConfiguration config = defaultEngineConfiguration)
		{
			configuration = config; 
		}
		
		input::InputManager inputManager;
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
				skyboxModel = assets::loadObj("assets/skybox.obj", {}, 1, false, false, false, false, false, false);
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

		void drawText(std::string text, float x, float y, VEC3 color = {1, 1, 1}, float scale = 1, TextAlign align = TextAlign::alignLeft);
		void drawText(std::string text, float x, float y, float z, VEC3 color, float scale, TextAlign align = TextAlign::alignLeft); 

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
		virtual void onCreateEntity(EntityId entityId) override;
		virtual void onRemoveEntity(EntityId entityId) override;

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
		void initInputManager();

		// pipelines
		void initPipelines(RenderSet& set);
		void recreatePipelines(RenderSet& set);
		VkDescriptorSetLayout initDescriptorSetLayout(Material material);
		PipelineInfo initGraphicsPipeline(RenderPass renderPass, Material material);

		// renderers
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
				if (entity.index >= 0 && entity.components & renderPrototype)
				{
					MeshId meshId = e_mesh_ids[entity.index];
					assert(meshId >= 0); 

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
						assert(mesh->materialId >= 0);
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
			
			invalidateComponents(ALL_COMPONENTS);

			set.isPrepared = true;
			set.isInvalidated = true; 
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
			set.isInitialized = true; 

			END_TIMER("Renderset:\n- vertexCount:   %d\n- indexCount:    %d\n- instanceCount: %d\n- materialCount: %d\n- meshCount:     %d\n- time:          ", set.vertexCount, set.indexCount, set.instanceCount, set.usedMaterialIds.size(), set.meshes.size())
			return set; 
		}

		void updateRenderSet(RenderSet& set, MAT4 view, MAT4 projection, float far)
		{
			if (set.recreatePipelines)
			{
				recreatePipelines(set); 
				set.recreatePipelines = false; 
			}

			// if invalidated, rebuild before the cull because of changing meshoffsets 
			if (!set.isPrepared)
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
				frustumChanged
				||
				firstCull /* first init */ ))
			{
				return;
			}

			firstCull = false; 
			set.meshDisposals.clear(); 

			Frustum frustum = Frustum(vp);

			int instanceOffset = 0;
			int totalInstanceCount = 0; 
			VEC3 eye = cameraController.getPosition(); 

			std::array<std::vector<EntityId>, LOD_LEVELS> lodData;

			ENTITY_ID* entityIndices = (ENTITY_ID*)getComponentData(ct_render_index);

			VEC4* positions = (VEC4*)getComponentData(ct_position);
			QUAT* rotations = (QUAT*)getComponentData(ct_rotation);
			VEC4* scales = (VEC4*)getComponentData(ct_scale);
			VEC4* colors = (VEC4*)getComponentData(ct_color);
			BBOX* bboxs = (BBOX*)getComponentData(ct_boundingBox);
			FLOAT* distances = (FLOAT*)getComponentData(ct_distance);

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
							FLOAT distance = distances[entityId];
							bool visible = distance >= -CHUNK_SIZE_Y / 2; 

							if (visible)
							{
								BBOX box = bboxs[entityId];
								visible = frustum.IsBoxVisible(box.min, box.max);
							}

							if (visible)
							{
								if (!mesh->isLoaded() && mesh->requestMesh != nullptr && mesh->userdataPtr != nullptr)
								{
									// request mesh data for next frame, closest should be loaded first 									
									set.meshRequests.push_back({ entityId, mesh->meshId, distance });								
								}
								else
								{
									// add to render data
									UINT lodLevels = mesh->lods.size(); 
									distance = ABS(distance); 

									if (configuration.enableLOD && lodLevels > 1)
									{
										haslods = true;
										UINT lodLevel = lodLevels - 1;

										if (mesh->cullDistance == 0 || mesh->cullDistance > distance)
										{
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
											instanceCount++;
										}
									}
									else
									{
										if (mesh->cullDistance == 0)
										{
											// all the same lod if disabled (could write data directly)
											lodData[0].push_back(entityId);
											instanceCount++;
										}
										else
										{
											if (distance < mesh->cullDistance)
											{
												lodData[0].push_back(entityId);
												instanceCount++;
											}
										}
									}									
								}
							}
							else
							{
								// not visible and requestable, might remove from set if distant 
								// these will be removed from the buffers if room is needed in this frame
								if (mesh->isLoaded() && mesh->requestMesh != nullptr && mesh->userdataPtr)
								{
									if (ABS(distance) > far * 0.7f)
									{											
										set.meshDisposals.push_back({ entityId, mesh->meshId, ABS(distance), MAX(mesh->quantized.size(), mesh->vertices.size()), mesh->indices.size() });
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

										memcpy(&entityIndices[rinfo.command.firstInstance], lodData[lodIndex].data(), sizeof(ENTITY_ID) * lodData[lodIndex].size());
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

								memcpy(&entityIndices[rinfo.command.firstInstance], lodData[0].data(), sizeof(ENTITY_ID) * lodData[0].size());
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

			invalidateComponents(ct_render_index); 
			openMeshRequests = set.meshRequests.size(); 
		}

		void updateIndirectRenderInfo(RenderSet& renderSet, UINT frame, bool force = false); 

		// frame update
		void updateFrameData(RenderSet& set, uint32_t currentFrame, SceneInfoBufferObject& sceneInfo)
		{
			// scene information
			memcpy(sceneInfoBuffers[currentFrame].mappedData, &sceneInfo, sizeof(SceneInfoBufferObject));

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

				if (set.isPrepared)
				{
					if (!tryAppendMeshToSetBuffers(set, mesh, req.entityId))
					{
						set.isPrepared = false;
					}
				}
				
				requestIndex++;
			}
			// keep running for more meshes if we introduce a complete buffer invalidation
			while (requestIndex < 10 && requestIndex < set.meshRequests.size() && !set.isPrepared);

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

			updateSystems(currentFrame); 
			
			updateRenderSet(renderSet, sceneInfo.view, sceneInfo.proj, cameraController.getIntrinsic().far);
			updateFrameData(renderSet, currentFrame, sceneInfo);
			updateIndirectRenderInfo(renderSet, currentFrame); 

			updateUI(deltaTime); 	
			updateGrid();
			updateTextOverlay(); 
			
			syncDirty(currentFrame);

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





