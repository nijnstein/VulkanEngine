#pragma once

namespace vkengine
{
	class DebugGrid
	{
	private:
		struct CameraUbo
		{
			alignas(16) MAT4 projection;
			alignas(16) MAT4 view;
			alignas(16) MAT4 model;
			alignas(16) MAT4 viewProjectionInverse;
		};

		PipelineInfo pipelineInfo;
		CameraUbo ubo;
		Buffer uboBuffer;

	public:
		float lineWidth = 1;

		void init(VulkanDevice* vulkanDevice, RenderPass& renderPass);
		void update(MAT4 projection, MAT4 view, MAT4 model, VEC3 transform = VEC3(0, 0, 0)); 
		void draw(VulkanDevice* device, VkCommandBuffer& commandBuffer, UINT currentFrame);
		void destroy();
	};
}