#include "defines.h"

namespace vkengine
{
	void DebugGrid::init(VulkanDevice* device, RenderPass& renderPass)
	{
		pipelineInfo = {};

		// create ubo buffer
		device->createBuffer(
			sizeof(CameraUbo), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, 
			uboBuffer, 
			"grid-ubo");

		// create layout
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
			init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0u),
			init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1u)
		};

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = init::descriptorSetLayoutCreateInfo(set_layout_bindings);
		descriptor_set_layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		VK_CHECK(vkCreateDescriptorSetLayout(device->device, &descriptor_set_layout_create_info, nullptr, &pipelineInfo.descriptorSetLayout));

		VkPushConstantRange push_constant_range = init::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), 0);

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = init::pipelineLayoutCreateInfo(&pipelineInfo.descriptorSetLayout);

		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;

		VK_CHECK(vkCreatePipelineLayout(device->device, &pipeline_layout_create_info, nullptr, &pipelineInfo.pipelineLayout));

		// setup descriptors
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			std::vector<VkWriteDescriptorSet> descriptorSets;
			
			descriptorSets.push_back({
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = 0,
				.dstBinding = (uint32_t)descriptorSets.size(),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uboBuffer.descriptor
				});
						
			pipelineInfo.descriptors.push_back(descriptorSets);
		}

		// create pipeline
		VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
			init::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
				0, 
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterization_state =
			init::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
				0);

		VkPipelineColorBlendAttachmentState blend_attachment_state =
			init::pipelineColorBlendAttachmentState(
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				VK_TRUE);

		blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		//blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		//blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;		
		blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		VkPipelineColorBlendStateCreateInfo color_blend_state =
			init::pipelineColorBlendStateCreateInfo(1, &blend_attachment_state);

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
			init::pipelineDepthStencilStateCreateInfo(
				VK_TRUE,
				VK_FALSE,
				VK_COMPARE_OP_LESS); 

		VkPipelineViewportStateCreateInfo viewport_state =
			init::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisample_state =			
			init::pipelineMultisampleStateCreateInfo(device->getCurrentMSAASamples(), 0);

		std::vector<VkDynamicState> dynamic_state_enables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state =
			init::pipelineDynamicStateCreateInfo(
				dynamic_state_enables.data(),
				static_cast<uint32_t>(dynamic_state_enables.size()),
				0);

		const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
			init::vertexInputBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
			init::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_state = init::pipelineVertexInputStateCreateInfo();

		std::array< VkPipelineShaderStageCreateInfo, 2> shaderCI;
		shaderCI[0] = device->initVertexShader("compiled shaders/grid.vert.spv").createInfo();
		shaderCI[1] = device->initFragmentShader("compiled shaders/grid.frag.spv").createInfo();

		VkGraphicsPipelineCreateInfo graphics_create{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		graphics_create.pNext = VK_NULL_HANDLE;
		graphics_create.renderPass = renderPass.getRenderPass();
		graphics_create.pInputAssemblyState = &input_assembly_state;
		graphics_create.pRasterizationState = &rasterization_state;
		graphics_create.pColorBlendState = &color_blend_state;
		graphics_create.pMultisampleState = &multisample_state;
		graphics_create.pViewportState = &viewport_state;
		graphics_create.pDepthStencilState = &depth_stencil_state;
		graphics_create.pDynamicState = &dynamic_state;
		graphics_create.pVertexInputState = &vertex_input_state;
		graphics_create.pTessellationState = VK_NULL_HANDLE;
		graphics_create.stageCount = 2;
		graphics_create.pStages = shaderCI.data();
		graphics_create.layout = pipelineInfo.pipelineLayout;

		vkCreateGraphicsPipelines(device->device, device->pipelineCache, 1, &graphics_create, VK_NULL_HANDLE, &pipelineInfo.pipeline);
	}		

	void DebugGrid::update(MAT4 projection, MAT4 view, MAT4 model, VEC3 transform)
	{
		model = glm::translate(model, -transform); 

		ubo.model = model;
		ubo.view = view; 
		ubo.projection = projection; 
		ubo.viewProjectionInverse = glm::inverse(projection * view);

		memcpy(uboBuffer.mappedData, &ubo, sizeof(CameraUbo)); 
	}

	void DebugGrid::draw(VulkanDevice* device, VkCommandBuffer& commandBuffer, uint32_t currentFrame)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.pipeline);
		
		device->vkCmdPushDescriptorSetKHR(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineInfo.pipelineLayout,
			0,
			(uint32_t)pipelineInfo.descriptors[currentFrame].size(),
			pipelineInfo.descriptors[currentFrame].data());
		
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void DebugGrid::destroy()
	{

	}
};
