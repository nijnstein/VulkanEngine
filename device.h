#pragma once

namespace vkengine
{	
	class VulkanDevice; 
	class VulkanEngine; // forward

	struct PipelineInfo; 
	struct RenderSet;

	

	class VulkanDevice : public ITextureManager, public IMaterialManager
	{
	private:
		const std::string pipelineCacheFilename = "pipeline_cache_data.bin";

		VkSampleCountFlagBits maxMSAASamples;
		VkSampleCountFlagBits currentMSAASamples;

		uint32_t currentFrame = 0;
		bool framebufferResized;

	public:
		EngineConfiguration configuration;

		struct frameStatistics {
			FLOAT frameTime{ 0 };
			UINT triangleCount{ 0 };
			UINT instanceCount{ 0 };
			UINT drawCount{ 0 }; 
			UINT frameCounter{ 0 };
			std::array<uint32_t, LOD_LEVELS> lodCounts{};
		} frameStats;

		// instance/device 
		GLFWwindow* window;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VmaAllocator allocator;
		VkPhysicalDeviceProperties gpuProperties;
		VkPhysicalDeviceMemoryProperties memoryProperties;

		// swapchain 
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		Image depthImage;
		Image colorImage;

		// pipeline cache
		VkPipelineCache pipelineCache; 

		// commands/sync 
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;
		VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		// extensions
		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR{ VK_NULL_HANDLE };
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR{ VK_NULL_HANDLE };

		// precalculated pbr data for shaders
		Image brdfLUT{};
		Image irradianceCube{};
		Image prefilteredCube{};

		// assets
		std::map<ShaderId, ShaderInfo> shaders{};
		std::map<MaterialId, Material> materials{};
		std::map<TextureId, TextureInfo> textures{};
		std::map<MeshId, MeshInfo> meshes{};
		std::map<ModelId, ModelInfo> models{};

		//PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT{ VK_NULL_HANDLE };
		//PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT{ VK_NULL_HANDLE };
		//PFN_vkCmdSetLineWidth vkCmdSetLineWidth{ VK_NULL_HANDLE };

		VkCommandBuffer beginCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		void endCommandBuffer(VkCommandBuffer commandBuffer);
		void endCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue); 

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, Buffer& buffer, const char* debugname = nullptr, float reserve = 1.0f);
		void createBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, Buffer& buffer, const char* debugname = nullptr, float reserve = 1.0f);

		Image allocateImage(VkImageCreateInfo imgCreateInfo, const char* debugname = nullptr);
		Image allocateStagedImage(VkImageCreateInfo imgCreateInfo, bool autoGenerateMipmaps, void* data, uint32_t dataSize, VkFormat dataFormat = VK_FORMAT_R8G8B8A8_SRGB, const char* debugname = nullptr);

		Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const char* debugname = nullptr);
		Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, void* data, uint32_t dataSize, const char* debugname = nullptr);
		Image createCubemap(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const char* debugname = nullptr);

		VkImageView createImageView(VkImage img, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipmapLevels = 1, uint32_t layerCount = 1);
		void createImageView(Image& image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
		void createImageSampler(Image& image);
		void createImageSampler(Image& image, VkSamplerCreateInfo info);

		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		TextureInfo initTexture(std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool ensureWithView = false, bool ensureWithSampler = false, bool isColor = true, bool isCube = false);
		TextureInfo& getTexture(int32_t textureId, bool ensureWithView = false, bool ensureWithSampler = false);
		TextureInfo& getTexture(std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool ensureWithView = false, bool ensureWithSampler = false, bool isColor = true);
		void freeTexture(int32_t textureId);

		void registerModel(ModelInfo& model);
		MeshId registerMesh(MeshInfo mesh);
		TextureInfo& registerTexture(std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool isColor = true, bool isCube = false);

		MeshInfo& getMesh(MeshId id); 
		void setMesh(MeshInfo mesh); 
		void ensureMeshHashLOD0(MeshInfo& mesh);

		Material initMaterial(std::string name, int albedoId, int normalId, int metallicId, int roughnessId, int vertexShaderId, int fragmentShaderId);
		MaterialId registerMaterial(Material& material) override;
		MaterialBuilder* initMaterial(std::string name) override;
		Material getMaterial(MaterialId material);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount = 1);
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount = 1);
		void transitionImageLayout(VkCommandBuffer commandBuffer, Image& image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void transitionImageLayout(Image& image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void transitionImageLayout(Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange);
		void transitionImageLayout(VkCommandBuffer commandBuffer, Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange);

		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			Image& image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			Image& image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		Image generateBRDFLUT();
		Image generateIrradianceCube(ModelData* skyboxModel, Image environmentCube);
		Image generatePrefilteredCube(ModelData* skyboxModel, Image environmentCube);

		ModelInfo loadObj(const char* path, Material material, float scale = 1.0f, bool computeNormals = true, bool removeDuplicateVertices = true, bool absoluteScaling = false, bool computeTangents = true);
		ImageRaw loadRawImageData(const char* path);

		ShaderInfo initShader(const char* path, VkShaderStageFlagBits flags);
		ShaderInfo initVertexShader(const char* path);
	    ShaderInfo initFragmentShader(const char* path);

		VkSampleCountFlagBits getMaxMSAASamples() const;
		VkSampleCountFlagBits getCurrentMSAASamples() const;
		uint32_t getCurrentFrame() const;
		bool hasFrameBufferResized() const;
		VkFormat getSwapChainFormat();
		void invalidateFrameBuffer(); 

	protected:
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

		void initDeviceProperties(); 
		std::vector<const char*> getRequiredExtensions();

		void initInstance();
		void initVMA();
		void initWindow();
		void initDebugMessenger();
		void initPhysicalDevice();
		void initLogicalDevice();
		void initExtensions();
		void initCommandPool();
		void initPushDescriptors();
		void initCommandBuffers();
		void initSyncObjects();
		
		void initPBR(ModelData* skybox, Image& environmentCube);
		void cleanupPBR(); 

		void initSwapChain();
		void initSwapChainBuffers(VkRenderPass renderPass);
		void recreateSwapChain(VkRenderPass renderPass);
		void cleanupSwapChain();

		void initIndirectCommandBuffers(PipelineInfo& pipeline); 
	    void destroyIndirectCommandBuffers(PipelineInfo& pipeline);
		void invalidateDrawCommandBuffers(PipelineInfo& pipeline);
		void invalidateDrawCommandBuffers(RenderSet& set);

		void initPipelineCache();     
		void destroyPipelineCache();

		ImageRaw loadKtxRaw(const char* name); 
		Image loadKtx(const std::string& name, const std::vector<uint8_t>& data, VkFormat format, bool isColor);

		void resetFrameStats();
		void updateFrameStats(UINT instanceCount, UINT triangleCount, UINT lodLevel = 0);
		void updateFrameStatsDrawCount(UINT drawCount);

		// handle resize updates
		virtual void updateAfterResize() = 0;
	};

}