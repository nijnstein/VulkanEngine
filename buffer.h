#pragma once
#include "defines.h"

namespace vkengine
{
	struct Buffer
	{
		const char* name; 
		VkBuffer buffer;
		VmaAllocation alloc;
		VkBufferCreateInfo info;
		void* mappedData;

		VkDescriptorBufferInfo descriptor; 

		void updateDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			descriptor.buffer = buffer;
			descriptor.offset = offset;
			descriptor.range = size;
		}

		bool isAllocated() const { return buffer != VK_NULL_HANDLE; }
	};
}