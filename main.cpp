#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "world.h"
#include "player.h"

using namespace vkengine; 


class TestApp : public vkengine::VulkanEngine
{
	World world;
	Player player;

	WINDOW_ID statsWindow;
	WINDOW_ID drawCommandsWindow;
	WINDOW_ID entityInfoWindow;
	WINDOW_ID consoleWindow;


	int nx = VERBOSE ? 14 : 200;
	int nz = VERBOSE ? 14 : 200;

	// index = x * nz + z

	void initScene()
	{
		reserve((nx + 1) * (nz + 1) + 1);

		addSystem(
			"bbox-from-aabb",
			ct_position | ct_scale | ct_mesh_id,
			ct_boundingBox,
			system_bbox_from_aabb);

		addSystem(
			"distance-from-bbox",
			ct_camera | ct_boundingBox,
			ct_distance,
			system_distance_from_bbox);

		addSystem(
			"physics",
			ct_mass | ct_collider | ct_linear_velocity | ct_radial_velocity,
			ct_none,
			system_physics);

		auto lineMaterial = initMaterial("wireframe")
			->setFragmentShader(initFragmentShader(WIREFRAME_FRAG_SHADER).id)
			->setVertexShader(initVertexShader(WIREFRAME_VERT_SHADER).id)
			->setTopology(MaterialTopology::LineList)
			->setLineWidth(3)
			->build();

		auto cylinder = loadObj("assets/cylinder.obj", getMaterial(lineMaterial));
		auto cId = fromModel(cylinder, false, renderPrototype);
		setComponentData(cId, ct_position, VEC4(0, 0, 0, 0));
		setComponentData(cId, ct_scale, VEC4(10));

		world.initChunks(this, { 0, 0 }, { nx, nz });
		player.init(this, &world);

		statsWindow = addWindow(new FrameStatisticsWindow({ 20, 20 }, { 560, 310 }, true, true));
		drawCommandsWindow = hideWindow(addWindow(new DrawCommandsWindow({ 20, 400 }, { 560, 280 }, true, true)));
		entityInfoWindow = hideWindow(addWindow(new EntityInfoWindow({ 20, 20 }, { 560, 310 }, true, true)));
		consoleWindow = hideWindow(addWindow(new ConsoleWindow()));
	}

	static void system_physics(EntityIterator* it)
	{
		VulkanEngine* engine = (VulkanEngine*)it->userdata;

		bool updates = false;

		VEC4* position = (VEC4*)engine->getComponentData(ct_position);
		FLOAT* mass = (FLOAT*)engine->getComponentData(ct_mass);
		VEC4* velocity = (VEC4*)engine->getComponentData(ct_linear_velocity);

		while (it->next())
		{

			// every object with mass gets to move  


		}

		if (updates)
		{
			engine->invalidateComponents(ct_position | ct_linear_velocity);
		}
	}

	static void system_distance_from_bbox(EntityIterator* it)
	{
		VulkanEngine* engine = (VulkanEngine*)it->userdata;

		BBOX* boxs = (BBOX*)engine->getComponentData(ct_boundingBox);
		FLOAT* distances = (FLOAT*)engine->getComponentData(ct_distance);

		VEC3 eye = engine->cameraController.getPosition();
		VEC3 forward = engine->cameraController.getForward();

		while (it->next(EntityIterator::Options::IncludeStatic))
		{
			int id = it->cursor->index;

			// calculate new distance from eye 
			BBOX box = boxs[id];
			FLOAT distance = 0;

			VEC3 center = box.Center();
			VEC3 pos = VEC3(center.x, eye.y, center.z);
			FLOAT dist = ABS(DISTANCE(eye, pos));
			VEC3 dir = NORM(center - eye);

			// distance = negative if behind the camera's forward vector 
			dist = DOT(forward, dir) < 0 ? -dist : dist;


			distances[id] = dist;
		}
	}

	static void system_bbox_from_aabb(EntityIterator* it)
	{
		VulkanEngine* engine = (VulkanEngine*)it->userdata;

		VEC3* pos = (VEC3*)engine->getComponentData(ct_position);
		VEC3* scale = (VEC3*)engine->getComponentData(ct_scale);
		BBOX* boxs = (BBOX*)engine->getComponentData(ct_boundingBox);
		MeshId* meshids = (MeshId*)engine->getComponentData(ct_mesh_id);

		MeshId last = -1;
		AABB aabb{};

		while (it->next())
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

		// after this, should update colliders on non static entities 
	}


	void initCamera()
	{
		vkengine::CameraExtrinsic e{};
		e.position = player.getPosition();
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
		invalidateComponents(ct_camera);
	}

	void updateFrame(SceneInfoBufferObject& sceneInfo, float deltaTime)
	{
		player.update(deltaTime);

		sceneInfo.lightPosition = VEC3(50, -100, -100);
		sceneInfo.lightDirection = glm::normalize(sceneInfo.lightPosition);
		sceneInfo.lightColor = VEC3(1, 1, 1);
		sceneInfo.lightIntensity = 1;

		drawText("NijnEngine, Press C to toggle camera mode", 20, 30, { 0, 1, 0 }, 1);

		// show player position
		VEC3 pos = player.getPosition();
		VEC3 forward = player.getForward();
		IVEC2 chunkXZ = world.getChunkXZFromWorldXYZ(pos);

		VEC3 camPos = cameraController.getPosition();

		std::stringstream s;

		s << "Player chunkXZ: " << chunkXZ.x << ", " << chunkXZ.y;
		drawText(s.str().c_str(), 20, 70, { 0, 0, 1 }, 1);

		s = {};
		s << "Player position: " << pos.x << ", " << pos.y << ", " << pos.z;
		drawText(s.str().c_str(), 20, 100, { 0, 0, 1 }, 1);

		s = {};
		s << "Player forward: " << forward.x << ", " << forward.y << ", " << forward.z;
		drawText(s.str().c_str(), 20, 130, { 0, 0, 1 }, 1);

		s = {};
		s << "Camera position: " << camPos.x << ", " << camPos.y << ", " << camPos.z;
		drawText(s.str().c_str(), 20, 160, { 0, 0, 1 }, 1);
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