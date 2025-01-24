#pragma once

namespace vkengine
{
	class DrawCommandsWindow : public ui::Window
	{
	public:
		DrawCommandsWindow(VEC2 _position, IVEC2 _size, bool _persistSize, bool _persistPosition)
		{
			init("Draw Commands", _position, _size, _persistSize, _persistPosition);
		}

		void onResize(VulkanEngine* engine) override
		{
		}

		void onRender(VulkanEngine* engine, float deltaTime) override
		{
		    // 
			// render data
			// 
			int c = 0;
			for (auto& [mid, p] : engine->getRenderSet()->pipelines)
			{
				for (auto& ri : p.culledRenderInfo)
				{
					ImGui::Text("mesh: %4d  lod: %4d  index: %8d  count: %6d  instance count: %4d  offset: %d",
						ri.meshId,
						ri.lodLevel,
						ri.command.firstIndex,
						ri.command.indexCount,
						ri.command.instanceCount,
						ri.command.firstInstance);

					if (++c >= 10) break;
				}
				if (c >= 10) break;
			}
		}
	};

	class FrameStatisticsWindow : public ui::Window
	{
	public:
		FrameStatisticsWindow(VEC2 _position, IVEC2 _size, bool _persistSize, bool _persistPosition)
		{
			init("Frame Statistics", _position, _size, _persistSize, _persistPosition);
		}

		void onResize(VulkanEngine* engine) override
		{
		}

		void onRender(VulkanEngine* engine, float deltaTime) override
		{
			auto frameStats = engine->frameStats; 

			ImGui::Begin("Frame Statistics");
			ImGui::SetWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
			ImGui::SetWindowSize(ImVec2(560, 335), ImGuiCond_Always);

			// Update frame time display
			float min = 0, max = 0;
			for (auto time : engine->uiSettings.frameTimes)
			{
				min = min == 0 ? time : fmin(time, min);
				max = max == 0 ? time : fmax(time, max);
			}
			ImGui::Text("Frame Times, max overall: %.02fms, min/max last 5sec: %.02fms / %.02fms", engine->uiSettings.frameTimeMax * 1000.0f, min * 1000.0f, max * 1000.0f);
			ImGui::PlotLines(" ", &engine->uiSettings.frameTimes[0], engine->uiSettings.frameTimes.size(), 0, "", min, max, ImVec2(540, 100));

			auto ci = engine->cameraController.getExtrinsic();
			switch (engine->cameraController.getCameraType())
			{
			case CameraType::Fps:
				ImGui::Text("Camera [FPS mode]");
				ImGui::InputFloat3("position", &ci.position.x);
				ImGui::InputFloat3("lookat", &ci.lookatTarget.x);
				if (ImGui::Button("Switch to ArcBall mode"))
				{
					engine->cameraController.setCameraType(CameraType::ArcBall);
				}
				break;

			case CameraType::ArcBall:
				ImGui::Text("Camera [ArcBall mode]");
				ImGui::InputFloat3("position", &ci.position.x);
				ImGui::InputFloat3("lookat", &ci.lookatTarget.x);
				if (ImGui::Button("Switch to FPS mode"))
				{
					engine->cameraController.setCameraType(CameraType::Fps);
				}
				break;
			}

			//# GRID
			ImGui::SameLine();
			if (ImGui::Button(engine->configuration.enableGrid ? "Disable Grid" : "Enable Grid"))
			{
				engine->configuration.enableGrid = !engine->configuration.enableGrid;
			}

			//# CULL 
			switch (engine->configuration.cullingMode)
			{
			case CullingMode::disabled:
				if (ImGui::Button("Enable culling"))
				{
					engine->configuration.cullingMode = CullingMode::full;
					engine->invalidate();
				}
				break;
			case CullingMode::full:
				if (ImGui::Button("Disable culling"))
				{
					engine->configuration.cullingMode = CullingMode::disabled;
					engine->invalidate();
				}
				break;
			}

			//# indirect 
			ImGui::SameLine();
			if (engine->configuration.enableIndirect)
			{
				if (ImGui::Button("Disable Indirect Rendering"))
				{
					engine->disableIndirectRendering(); 
				}
			}
			else
			{
				if (ImGui::Button("Enable Indirect Rendering"))
				{
					engine->enableIndirectRendering();
				}
			}

			//# wireframe mode
			ImGui::SameLine();
			if (engine->configuration.enableWireframe)
			{
				if (ImGui::Button("Solid"))
				{
					engine->disableWireframeRendering();
				}
			}
			else
			{
				if (ImGui::Button("Wireframe"))
				{
					engine->enableWireframeRendering();
				}
			}


			//# vsync
			ImGui::SameLine();
			if (engine->configuration.enableVSync)
			{
				if (ImGui::Button("Disable VSync"))
				{
					engine->disableVSync();
				}
			}
			else
			{
				if (ImGui::Button("Enable VSync"))
				{
					engine->enableVSync();
				}
			}


			if (engine->configuration.cullingMode == CullingMode::full)
			{			 
				ImGui::Text("Rendering %d of %d entities, #triangles: %d", 
					engine->getRenderSet()->culledInstanceCount,
					engine->getRenderSet()->instanceCount,
					engine->frameStats.triangleCount);

				//# LOD (only if also culled)
				if (ImGui::Button(engine->configuration.enableLOD ? "Disable LOD" : "Enable LOD"))
				{
					engine->configuration.enableLOD = !engine->configuration.enableLOD;
					engine->invalidate();
				}
				if (engine->configuration.enableLOD)
				{
					for (int i = 0; i < LOD_LEVELS; i++)
					{
						ImGui::SameLine();
						ImGui::Text("LOD%d: %d", i, frameStats.lodCounts[i]);
					}
				}
			}
			else
			{
				ImGui::Text("Rendering %d entities, #triangles: %d", engine->getRenderSet()->instanceCount, frameStats.triangleCount);
			}
			ImGui::End();

		}

	};

	class EntityInfoWindow : public ui::Window
	{
	public:
		EntityInfoWindow(VEC2 _position, IVEC2 _size, bool _persistSize, bool _persistPosition)
		{
			init("Entity Information", _position, _size, _persistSize, _persistPosition);
		}

		void onResize(VulkanEngine* engine) override
		{
		}

		void onRender(VulkanEngine* engine, float deltaTime) override
		{
			static EntityId id = 0;

			ImGui::Text("EntityId:"); ImGui::SameLine();
			ImGui::InputInt("EntityId:", &id);
			ImGui::InputFloat3("Position: ", (float*)engine->getComponentData(id, ct_position));
			ImGui::InputFloat4("Rotation: ", (float*)engine->getComponentData(id, ct_rotation));
			ImGui::InputFloat3("Scale: ", (float*)engine->getComponentData(id, ct_scale));
			ImGui::InputFloat3("BBox min: ", &((BBOX*)engine->getComponentData(id, ct_boundingBox))->min[0]);
			ImGui::InputFloat3("BBox max: ", &((BBOX*)engine->getComponentData(id, ct_boundingBox))->max[0]);

			MeshInfo& meshinfo = engine->getMesh(*(int*)engine->getComponentData(id, ct_mesh_id));
			ImGui::InputInt("Mesh: ", (int*)engine->getComponentData(id, ct_mesh_id));
			ImGui::InputFloat3("Mesh AABB min: ", &meshinfo.aabb.min[0]);
			ImGui::InputFloat3("Mesh AABB max: ", &meshinfo.aabb.max[0]);
			ImGui::Text("Vertexcount: %d, indexcount: %d, triangles: %d", meshinfo.vertices.size(), meshinfo.indices.size(), meshinfo.indices.size() / 3);

			ImGui::InputInt("Material: ", (int*)engine->getComponentData(id, ct_material_id));

			Chunk* chunk = (Chunk*)engine->getComponentData(id, ct_chunk);
			if (chunk != nullptr)
			{
				ImGui::InputInt("Chunk: ", &chunk->chunkId);
				ImGui::InputFloat2("Chunk Grid XY", &chunk->gridXZ[0]);
				ImGui::InputFloat3("Chunk min: ", &chunk->min[0]);
				ImGui::InputFloat3("Chunk max: ", &chunk->max[0]);
			}

			// check if culled or not 
			bool inSet = false;
			for (auto& [mid, p] : engine->getRenderSet()->pipelines)
			{
				for (auto& ri : p.culledRenderInfo)
				{
					if (id >= ri.command.firstInstance && id < ri.command.firstInstance + ri.command.instanceCount)
					{
						inSet = true;
						break;
					}
				}
				if (inSet) break;
			}

			if (inSet) ImGui::Text("<IN VISIBLE SET>");
			else ImGui::Text("<ENTITY NOT VISIBLE, CULLED>");
		}
	};
}