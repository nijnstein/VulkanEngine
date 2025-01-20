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
		reserveCpuBuffers((nx + 1) * (nz + 1));

 	    auto lineMaterial = initMaterial("line-material")->setTopology(MaterialTopology::LineList)->build(); 


		world.initChunks({ 0, 0 }, { nx, nz });
		world.setMaterialId(initMaterial
		(
			"phong-material",
			-1, -1, -1, -1,
			initVertexShader(PHONG_VERTEX_SHADER).id,
			initFragmentShader(PHONG_FRAGMENT_SHADER).id
		).materialId);
		
     	world.generateEntities(this);

		statsWindow = addWindow(new FrameStatisticsWindow({ 20, 20 }, { 560, 310 }, true, true));
		drawCommandsWindow = hideWindow(addWindow(new DrawCommandsWindow({ 20, 400 }, { 560, 280 }, true, true)));
		entityInfoWindow = hideWindow(addWindow(new EntityInfoWindow({ 20, 20 }, { 560, 310 }, true, true)));	
		consoleWindow = hideWindow(addWindow(new ConsoleWindow()));

		playerPosition = world.getGroundLevel({ 0.0f, 0.0f }) - 4.5f;
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
		i.far = 1000.0f;
		i.near = 0.01f;
		i.fov = 90;
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

		drawText("NijnEngine", 1000, 100); 
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