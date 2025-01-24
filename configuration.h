#pragma once

enum class CullingMode
{
	disabled,
	full
};

struct EngineConfiguration
{
	bool enableVSync = VERBOSE ? false : true;
	bool enableLOD = true;
	bool enablePBR = false;
	bool enableIndirect = true; 
	bool enableShadows = false; 
	bool enableWireframe = false; 
	bool enableGrid = true; 
	bool enableTextOverlay = true; 
	//bool enableChunkBorders = true; 

	CullingMode cullingMode = CullingMode::full;
	float textScale = 1.0f;


	//#
	//# Frame/Depth buffer 
	//# 
	std::array<VkFormat, 1> colorFormat = { VK_FORMAT_B8G8R8A8_SRGB };
	std::array<VkFormat, 3> depthFormat = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkClearColorValue clearColor = { 0.2f,0.2f, 0.7f, 1.0f };

	SIZE maxIndirectCommandCount = 1024 * 16; 

	VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_4_BIT;

	VkExtent2D frameBufferSize = { 1600, 800 };
	bool fullScreen = false;
	std::string windowName = "NijnEngine v0.0";

	//#
	//#	Entity vertex/data buffers 
	//#  
	UINT minEntityVertexBufferSize{ 1024 * 1024 * 512 };
	UINT minEntityIndexBufferSize{ 1024 * 1024 * 64 };

	//#
	//# Shadow mapping  (todo)
	//#
	bool displayShadowMap = false;
	bool filterPCF = true;

	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;

	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	VkFormat offscreenDepthFormat = VK_FORMAT_D16_UNORM;
	uint32_t shadowMapSize{ 4096 };

} const defaultEngineConfiguration;
