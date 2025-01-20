#pragma once

namespace vkengine
{
	typedef int32_t TextureId;

	struct TextureInfo
	{
		TextureId id;
		std::string path;
		Image image;
		VkFormat format;
		bool isColor; 
		bool isCube; 

		VkWriteDescriptorSet getDescriptor(uint32_t binding)
		{
			return {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = 0,
					.dstBinding = binding,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &image.descriptor
			};
		}
	};

	class ITextureManager
	{
	public:
		virtual ~ITextureManager() {};

		virtual TextureInfo& getTexture(int32_t textureId, bool ensureWithView = false, bool ensureWithSampler = false) = 0;
	};
}