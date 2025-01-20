#pragma once
#include "defines.h"

namespace vkengine
{
	struct Image
	{
		const char* name; 

		VkImage img;
		VmaAllocation allocation;
		VkImageView view;
		VkSampler sampler;

		UINT mipmapLevels;
		UINT layerCount; 
		UINT width;
		UINT height;
		UINT depth;
		VkFormat format;
		VkImageLayout layout;
		VkImageCreateFlags flags;
		bool isCube{ false };

		VkDescriptorImageInfo descriptor;

		void updateDescriptor()
		{
			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = layout;
		}
    };

	struct ImageRaw
	{
		const char* name; 

		BYTE* data; 
		UINT dataSize; 

		UINT width; 
		UINT height; 
		UINT depth; 

		void deallocate()
		{
			if (data)
			{
				free(data);
				data = nullptr;
			}
		}
	};
}