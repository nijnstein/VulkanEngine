#pragma once

namespace vkengine
{
	typedef int32_t ShaderId;

	struct ShaderInfo
	{
		ShaderId id;
		std::string path;
		VkShaderModule module;
		VkShaderStageFlagBits flags;

		VkPipelineShaderStageCreateInfo createInfo()
		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.stage = flags;
			info.module = module;
			info.pName = "main";
			return info; 
		}
	};
}
