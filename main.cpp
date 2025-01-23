#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "world.h"

using namespace vkengine; 


class TestApp : public vkengine::VulkanEngine
{
	World world; 

	WINDOW_ID statsWindow; 
	WINDOW_ID drawCommandsWindow; 
	WINDOW_ID entityInfoWindow; 
	WINDOW_ID consoleWindow; 

	VEC3 playerPosition; 
		
	int nx = VERBOSE ? 14 : 200;
	int nz = VERBOSE ? 14 : 200;

	// index = x * nz + z

	void initScene()
	{
		reserve((nx + 1) * (nz + 1) + 1);

		addSystem(
			"bbox-from_aabb_pos_scale",
			ct_position | ct_scale | ct_mesh_id,
			ct_boundingBox,
		    (ComponentSystem::Stage)(ComponentSystem::Stage::OnCreate | ComponentSystem::Stage::BeforeFrameUpdate),
			system_bbox_from_aabb);

 	    auto lineMaterial = initMaterial("wireframe")
			->setFragmentShader(initFragmentShader(WIREFRAME_FRAG_SHADER).id)
			->setVertexShader(initVertexShader(WIREFRAME_VERT_SHADER).id)
			->setTopology(MaterialTopology::LineList)
			->setLineWidth(3)
			->build();

		auto cylinder = loadObj("assets/cylinder.obj", getMaterial(lineMaterial)); 
		auto cId = fromModel(cylinder, false, renderPrototype);
		setComponentData(cId, ct_position, VEC4( 0, 0, 0, 0 ));
		setComponentData(cId, ct_scale, VEC4( 10 ));

		world.initChunks(this, { 0, 0 }, { nx, nz });

		statsWindow = addWindow(new FrameStatisticsWindow({ 20, 20 }, { 560, 310 }, true, true));
		drawCommandsWindow = hideWindow(addWindow(new DrawCommandsWindow({ 20, 400 }, { 560, 280 }, true, true)));
		entityInfoWindow = hideWindow(addWindow(new EntityInfoWindow({ 20, 20 }, { 560, 310 }, true, true)));	
		consoleWindow = hideWindow(addWindow(new ConsoleWindow()));

		playerPosition = world.getGroundLevel({ 0.0f, 0.0f }) - 1.5f;

		cId = fromModel(cylinder, false, renderPrototype);
		auto aabb = cylinder.aabb;
		auto pos = VEC4(world.getGroundLevel({ 0.0f, 0.0f }) + VEC3(0.5f, 0, 0.5f), 1); 
		auto scale = VEC4(1); 
		
		setComponentData(cId, ct_position, pos);
		setComponentData(cId, ct_scale, scale);		
	}
 

	static void system_bbox_from_aabb(EntityIterator* it)
	{
		VulkanEngine* engine = (VulkanEngine*)it->userdata; 

		VEC3*	pos			= (VEC3*)	engine->getComponentData(ct_position);
		VEC3*	scale		= (VEC3*)	engine->getComponentData(ct_scale); 
		BBOX*	boxs		= (BBOX*)	engine->getComponentData(ct_boundingBox);
		MeshId* meshids		= (MeshId*)	engine->getComponentData(ct_mesh_id); 
		
		MeshId last = -1;
		AABB aabb{}; 

		do
		{
			int id = it->cursor->index;

			VEC3 p = pos[id];
			VEC3 s = scale[id];
			
			MeshId meshId = meshids[id];
			if (meshId != last)
			{
				aabb = engine->getMesh(meshId).aabb;
				last = meshId;
			}

			boxs[id] = BBOX
			{
				VEC4(aabb.min * s + p, 1),
				VEC4(aabb.max * s + p, 1)
			};
		}
		while (it->next()); 		
	}

	void initCamera()
	{
		vkengine::CameraExtrinsic e{};
		e.position = playerPosition; 
		e.forward = glm::vec3(0.f, 0.f, 1.f);
		e.right = glm::vec3(1.f, 0.f, 0.f);
		e.up = glm::vec3(0.f, -1.f, 0.f);
		e.lookatTarget = VEC3(0, -120, 0);

		vkengine::CameraIntrinsic i{};
		i.far = 1500.0f;
		i.near = 0.01f;
		i.fov = 45;
		i.viewPortWidth = swapChainExtent.width;
		i.viewPortHeight = swapChainExtent.height;
		i.aspectRatio = i.viewPortWidth / i.viewPortHeight;

		cameraController.init(e, i, CameraType::Fps);
	}

	void updateFrame(SceneInfoBufferObject& sceneInfo, float deltaTime)
	{
		VEC3 pos = cameraController.getPosition();
		VEC3 ground = world.getGroundLevel({ pos.x, pos.z }) + VEC3(0, -1, 0);

		playerPosition = ground; 
		cameraController.setPosition(playerPosition); 

		sceneInfo.lightPosition = VEC3(50, -100, -100);
		sceneInfo.lightDirection = glm::normalize(sceneInfo.lightPosition);
		sceneInfo.lightColor = VEC3(1, 1, 1);
		sceneInfo.lightIntensity = 1;

		drawText("NijnEngine", 1000, 100, { 0, 1, 0 }, 1);
		drawText("NijnEngine", 0, 0, 0, { 1, 1, 0 }, 1);
	}

	void updateUIFrame() 
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Windows"))
			{
				if (ImGui::MenuItem("Frame Statistics"))
				{
					showWindow(statsWindow);
				}
				if (ImGui::MenuItem("Entity Information"))
				{
					showWindow(entityInfoWindow);
				}
				if (ImGui::MenuItem("Draw Commands"))
				{
					showWindow(drawCommandsWindow);
				}
				if (ImGui::MenuItem("Console"))
				{
					showWindow(consoleWindow);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("World"))
			{
				if (world.getChunkBordersEnabled())
				{
					if (ImGui::MenuItem("Disable Chunk Borders"))
					{
						world.disableChunkBorders(this); 
					}
				}
				else
				{
					if (ImGui::MenuItem("Enable Chunk Borders"))
					{
						world.enableChunkBorders(this);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	};

	void cleanupScene()
	{
	}
};



int main()
{
	//Application app {}; 
	TestApp app{}; 

	try
	{
		app.init(); 
		app.run();
		app.destroy();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}