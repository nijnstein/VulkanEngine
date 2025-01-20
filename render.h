#pragma once

namespace vkengine
{
	struct RenderInfo
	{
		VkDrawIndexedIndirectCommand command;

		MeshId meshId;
		int lodLevel;
		int vertexCount;
	};

	struct PipelineInfo
	{
		MaterialId materialId;

		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// descriptorsets foreach frame in flight 
		std::vector<std::vector<VkWriteDescriptorSet>> descriptors;

		// the visible objects in current frame using this pipeline
		std::vector<RenderInfo> culledRenderInfo;

		// indirect rendering buffers (one foreach flightframe) 
		std::vector<bool> indirectCommandBufferInvalidations;
		std::vector<Buffer> indirectCommandBuffers;
	};

	struct MeshOffsetInfo
	{
		MeshId meshId;
		MaterialId materialId;

		int vertexOffset;    // index of first vertex in lod0 of given mesh in given buffer 
		int vertexCount;     // the total number of vertices in the mesh including all lods 

		int indexOffset;	 // index into ibuffer[bufferIndex] [array]
		int indexCount;      // nr of indexes to render for this offset 

		int instanceOffset;  // offset of instance in entity data arrays (should normally match the entityid as long as no entity is removed) 

		int bufferIndex;     // index into v/i buffer arrays when using multiple vertex and index buffers 

		std::vector<EntityId> instances;
    };

	struct Chunk
	{
		EntityId chunkId; // == entity id

		VEC3 min;
		VEC3 max;
		VEC2 gridXZ;

		inline BBox getBox()
		{
			return { VEC4(min, 0), VEC4(max, 0) };
		}
	};

	// msg issued if a mesh is needed for render 
	// depending on room in i/b buffers this can be costly (todo: buffer chains?) 
	struct MeshRequest
	{
		EntityId entityId; 
		MeshId meshId; 
		float distance; 
	};

	// msg issued if a mesh may be removed from buffers
	// the actual remove is only processed if there is room needed in 
	// the i/b buffers 
	struct MeshDisposal
	{
		EntityId entityId;
		MeshId meshId;
		float distance;

		SIZE vertexCount;
		SIZE indexCount; 
	};


	struct RenderSet
	{
		bool isInvalidated{ true };

		UINT vertexCount; 
		UINT indexCount; 
		int instanceCount;
		int culledInstanceCount; 

		Buffer vertexBuffer;
		Buffer indexBuffer;

		bool invalidateVertexBuffers{ false };

		std::vector<MaterialId> usedMaterialIds;
		std::vector<MeshInfo*> meshes;

		std::map<MeshId, MeshOffsetInfo> meshOffsets;
		std::map<MeshId, std::vector<EntityId>> meshInstances;	 // contains all instances of meshes regardless if culled

		// offscreen pipelines for all materials 
		std::map<MaterialId, PipelineInfo> pipelines;


		// request/dispose requests of meshes 
		std::vector<MeshRequest> meshRequests;
		std::vector<MeshDisposal> meshDisposals;

		Buffer indirectCommandBuffer;
	};

	class RenderPass
	{
	private:
		VulkanDevice* device; 
		const char* debugName; 

		VkRenderPass renderPass; 

		void initDefault(VulkanDevice* vulkanDevice); 
		void initShadow(VulkanDevice* vulkanDevice);

		VkPipeline debug{ VK_NULL_HANDLE };
		VkPipeline sceneShadow{ VK_NULL_HANDLE };
		VkPipeline sceneShadowPCF{ VK_NULL_HANDLE };

	public:
		void init(VulkanDevice* vulkanDevice, const char* debugname = nullptr);
		void begin(VulkanDevice* vulkanDevice, VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkExtent2D extent);
		void end(VkCommandBuffer commandBuffer); 
		void destroy(); 

		VkRenderPass getRenderPass() const { return renderPass; }
	};
}