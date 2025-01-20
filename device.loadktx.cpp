#pragma once

#include "defines.h"
#include <ktxvulkan.h>	 

namespace vkengine
{
	namespace ktx
	{
		struct Mipmap
		{
			/// Mipmap level
			uint32_t level = 0;

			/// Byte offset used for uploading
			uint32_t offset = 0;

			/// Width depth and height of the mipmap
			VkExtent3D extent = { 0, 0, 0 };
		};

		struct CallbackData final
		{
			ktxTexture* texture;
			std::vector<Mipmap>* mipmaps;
		};
	}

	/// Row padding is different between KTX (pad to 4) and Vulkan (none).
	/// Also region->bufferOffset, i.e. the start of each image, has
	/// to be a multiple of 4 and also a multiple of the element size.
	static ktx_error_code_e KTX_APIENTRY optimal_tiling_callback(
		int          mip_level,
		int          face,
		int          width,
		int          height,
		int          depth,
		ktx_uint64_t face_lod_size,
		void* pixels,
		void* user_data)
	{
		auto* callback_data = reinterpret_cast<ktx::CallbackData*>(user_data);
		assert(static_cast<size_t>(mip_level) < callback_data->mipmaps->size() && "Not enough space in the mipmap vector");

		ktx_size_t mipmap_offset = 0;
		auto       result = ktxTexture_GetImageOffset(callback_data->texture, mip_level, 0, face, &mipmap_offset);
		if (result != KTX_SUCCESS)
		{
			return result;
		}

		auto& mipmap = callback_data->mipmaps->at(mip_level);
		mipmap.level = mip_level;
		mipmap.offset = (uint32_t)mipmap_offset;
		mipmap.extent.width = width;
		mipmap.extent.height = height;
		mipmap.extent.depth = depth;

		return KTX_SUCCESS;
	}

	// When the color-space of a loaded image is unknown (from KTX1 for example) we
	// may want to assume that the loaded data is in sRGB format (since it usually is).
	// In those cases, this helper will get called which will force an existing unorm
	// format to become an srgb format where one exists. If none exist, the format will
	// remain unmodified.
	static VkFormat maybe_coerce_to_srgb(VkFormat fmt)
	{
		switch (fmt)
		{
		case VK_FORMAT_R8_UNORM:
			return VK_FORMAT_R8_SRGB;
		case VK_FORMAT_R8G8_UNORM:
			return VK_FORMAT_R8G8_SRGB;
		case VK_FORMAT_R8G8B8_UNORM:
			return VK_FORMAT_R8G8B8_SRGB;
		case VK_FORMAT_B8G8R8_UNORM:
			return VK_FORMAT_B8G8R8_SRGB;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case VK_FORMAT_BC7_UNORM_BLOCK:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
			return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
			return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
			return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
			return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
			return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
			return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
			return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
			return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
			return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
			return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
			return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
			return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
			return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
			return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
			return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
			return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
			return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
			return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
		default:
			return fmt;
		}
	}

	ImageRaw VulkanDevice::loadKtxRaw(const char* name)
	{
		std::runtime_error("load ktx raw not implemented"); 
		return {}; 
	}

	Image VulkanDevice::loadKtx(const std::string& name, const std::vector<uint8_t>& data, VkFormat format, bool isColor)
	{
		std::vector<uint8_t> imagedata;
		std::vector<ktx::Mipmap> mipmaps{ {} };

		// Offsets stored like offsets[array_layer][mipmap_layer]
		std::vector<std::vector<VkDeviceSize>> offsets;
		
		Image image{};

		auto data_buffer = reinterpret_cast<const ktx_uint8_t*>(data.data());
		auto data_size = static_cast<ktx_size_t>(data.size());

		ktxTexture* texture;
		auto load_ktx_result = ktxTexture_CreateFromMemory(
			data_buffer,
			data_size,
			KTX_TEXTURE_CREATE_NO_FLAGS,
			&texture);

		if (load_ktx_result != KTX_SUCCESS)
		{
			throw std::runtime_error{ "Error loading KTX texture: " + name };
		}

		if (texture->pData)
		{
			imagedata.resize((uint32_t)texture->dataSize);
			memcpy(imagedata.data(), texture->pData, (size_t)texture->dataSize); 
		}
		else
		{
			// Load
			imagedata.resize((uint32_t)texture->dataSize);
			auto load_data_result = ktxTexture_LoadImageData(texture, imagedata.data(), (uint32_t)texture->dataSize);
			if (load_data_result != KTX_SUCCESS)
			{
				throw std::runtime_error{ "Error loading KTX image data: " + name };
			}
		}

		image.width = texture->baseWidth;
		image.height = texture->baseHeight;
		image.depth = texture->baseDepth;
		image.layerCount = texture->numLayers;

		bool cubemap = false;

		// Use the faces if there are 6 (for cubemap)
		if (texture->numLayers == 1 && texture->numFaces == 6)
		{
			cubemap = true;
			image.layerCount = texture->numFaces;
		}

		image.format = ktxTexture_GetVkFormat(texture);

		if (texture->classId == ktxTexture1_c && isColor)
		{
			// KTX-1 files don't contain color space information. Color data is normally
			// in sRGB, but the format we get back won't report that, so this will adjust it
			// if necessary.
			image.format = maybe_coerce_to_srgb(image.format);
		}

		mipmaps.resize(texture->numLevels);

		ktx::CallbackData callback_data{};
		callback_data.texture = texture;
		callback_data.mipmaps = &mipmaps;

		auto result = ktxTexture_IterateLevels(texture, optimal_tiling_callback, &callback_data);
		if (result != KTX_SUCCESS)
		{
			throw std::runtime_error("Error loading KTX texture");
		}

		// If the texture contains more than one layer, then populate the offsets otherwise take the mipmap level offsets
		if (texture->numLayers > 1 || cubemap)
		{
			uint32_t layer_count = cubemap ? texture->numFaces : texture->numLayers;

			for (uint32_t layer = 0; layer < layer_count; layer++)
			{
				std::vector<VkDeviceSize> layer_offsets{};
				for (uint32_t level = 0; level < texture->numLevels; level++)
				{
					ktx_size_t     offset;
					KTX_error_code result;
					if (cubemap)
					{
						result = ktxTexture_GetImageOffset(texture, level, 0, layer, &offset);
					}
					else
					{
						result = ktxTexture_GetImageOffset(texture, level, layer, 0, &offset);
					}
					layer_offsets.push_back(static_cast<VkDeviceSize>(offset));
				}
				offsets.push_back(layer_offsets);
			}
		}
		else
		{
			offsets.resize(1);
			for (size_t level = 0; level < mipmaps.size(); level++)
			{
				offsets[0].push_back(static_cast<VkDeviceSize>(mipmaps[level].offset));
			}
		}
		
		VkImageCreateInfo imageInfo{};

		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = image.width;
		imageInfo.extent.height = image.height;
		imageInfo.extent.depth = image.depth;
		imageInfo.mipLevels = mipmaps.size();
		imageInfo.arrayLayers = image.layerCount;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (texture->numLayers > 1 || cubemap) 
		{
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		image = allocateStagedImage(imageInfo, false, (void*)data.data(), texture->dataSize, format, name.c_str());
		

		ktxTexture_Destroy(texture);
		return image;
	}
}