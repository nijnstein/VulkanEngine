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
		
	int nx = VERBOSE ? 4 : 140;
	int nz = VERBOSE ? 4 : 140;

	// index = x * nz + z

	void initScene()
	{
		reserveCpuBuffers((nx + 1) * (nz + 1) + 1);

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
		auto pos = VEC4(world.getGroundLevel({ 0.0f, 0.0f }) + VEC3(0, 0, 0), 1); 
		auto scale = VEC4(1); 
		
		setComponentData(cId, ct_position, pos);
		setComponentData(cId, ct_scale, scale);		
	
		//
		// a system should do this on all non static objects in each frame and on creation 
		//

		setComponentData(cId, ct_boundingBox, BBOX
			{
				VEC4(aabb.min, 1) * scale + pos,
				VEC4(aabb.max, 1) * scale + pos
			});
	 
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
				if (world.chunkBordersEnabled())
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