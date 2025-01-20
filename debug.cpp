#include "defines.h" 

namespace vkengine
{
	namespace debug
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
			{
				std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
			}
			return VK_FALSE;
		}

        VkDebugUtilsMessengerCreateInfoEXT initDebugMessengerCreateInfo(void* pUserData)
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            createInfo.sType =
                VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = vkengine::debug::debugCallback;
            createInfo.pUserData = pUserData;
            return createInfo; 
        }

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const	VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}

			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				func(instance, debugMessenger, pAllocator);
			}
		}

		std::string errorString(VkResult errorCode)
		{
			switch (errorCode)
			{
#define STR(r) case VK_ ##r: return #r
				STR(NOT_READY);
				STR(TIMEOUT);
				STR(EVENT_SET);
				STR(EVENT_RESET);
				STR(INCOMPLETE);
				STR(ERROR_OUT_OF_HOST_MEMORY);
				STR(ERROR_OUT_OF_DEVICE_MEMORY);
				STR(ERROR_INITIALIZATION_FAILED);
				STR(ERROR_DEVICE_LOST);
				STR(ERROR_MEMORY_MAP_FAILED);
				STR(ERROR_LAYER_NOT_PRESENT);
				STR(ERROR_EXTENSION_NOT_PRESENT);
				STR(ERROR_FEATURE_NOT_PRESENT);
				STR(ERROR_INCOMPATIBLE_DRIVER);
				STR(ERROR_TOO_MANY_OBJECTS);
				STR(ERROR_FORMAT_NOT_SUPPORTED);
				STR(ERROR_SURFACE_LOST_KHR);
				STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
				STR(SUBOPTIMAL_KHR);
				STR(ERROR_OUT_OF_DATE_KHR);
				STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
				STR(ERROR_VALIDATION_FAILED_EXT);
				STR(ERROR_INVALID_SHADER_NV);
				STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
			default:
				return "UNKNOWN_ERROR";
			}
		}

		std::string imageLayoutString(VkImageLayout layout)
		{
			switch (layout)
			{
#define STR(r) case  ##r: return #r 
				STR(VK_IMAGE_LAYOUT_UNDEFINED);
				STR(VK_IMAGE_LAYOUT_GENERAL);
				STR(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_PREINITIALIZED);
				STR(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
				STR(VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ);
				STR(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR);
				STR(VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR);
				STR(VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT);
				STR(VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR);
				STR(VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR);
				STR(VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT);
				STR(VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR);
#undef STR
				default: return "unknown vk_image_layout"; 
			}
		}

		std::string formatString(VkFormat format)
		{
			switch (format)
			{
#define STR(r) case ##r: return #r; 
                    STR(VK_FORMAT_UNDEFINED)
                    STR(VK_FORMAT_R4G4_UNORM_PACK8)
                    STR(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
                    STR(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
                    STR(VK_FORMAT_R5G6B5_UNORM_PACK16)
                    STR(VK_FORMAT_B5G6R5_UNORM_PACK16)
                    STR(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
                    STR(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
                    STR(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
                    STR(VK_FORMAT_R8_UNORM)
                    STR(VK_FORMAT_R8_SNORM);
                    STR(VK_FORMAT_R8_USCALED);
                    STR(VK_FORMAT_R8_SSCALED);
                    STR(VK_FORMAT_R8_UINT);
                    STR(VK_FORMAT_R8_SINT);
                    STR(VK_FORMAT_R8_SRGB);
                    STR(VK_FORMAT_R8G8_UNORM);
                    STR(VK_FORMAT_R8G8_SNORM);
                    STR(VK_FORMAT_R8G8_USCALED);
                    STR(VK_FORMAT_R8G8_SSCALED);
                    STR(VK_FORMAT_R8G8_UINT);
                    STR(VK_FORMAT_R8G8_SINT);
                    STR(VK_FORMAT_R8G8_SRGB);
                    STR(VK_FORMAT_R8G8B8_UNORM);
                    STR(VK_FORMAT_R8G8B8_SNORM);
                    STR(VK_FORMAT_R8G8B8_USCALED);
                    STR(VK_FORMAT_R8G8B8_SSCALED);
                    STR(VK_FORMAT_R8G8B8_UINT);
                    STR(VK_FORMAT_R8G8B8_SINT);
                    STR(VK_FORMAT_R8G8B8_SRGB);
                    STR(VK_FORMAT_B8G8R8_UNORM);
                    STR(VK_FORMAT_B8G8R8_SNORM);
                    STR(VK_FORMAT_B8G8R8_USCALED);
                    STR(VK_FORMAT_B8G8R8_SSCALED);
                    STR(VK_FORMAT_B8G8R8_UINT);
                    STR(VK_FORMAT_B8G8R8_SINT);
                    STR(VK_FORMAT_B8G8R8_SRGB);
                    STR(VK_FORMAT_R8G8B8A8_UNORM);
                    STR(VK_FORMAT_R8G8B8A8_SNORM);
                    STR(VK_FORMAT_R8G8B8A8_USCALED);
                    STR(VK_FORMAT_R8G8B8A8_SSCALED);
                    STR(VK_FORMAT_R8G8B8A8_UINT);
                    STR(VK_FORMAT_R8G8B8A8_SINT);
                    STR(VK_FORMAT_R8G8B8A8_SRGB);
                    STR(VK_FORMAT_B8G8R8A8_UNORM);
                    STR(VK_FORMAT_B8G8R8A8_SNORM);
                    STR(VK_FORMAT_B8G8R8A8_USCALED);
                    STR(VK_FORMAT_B8G8R8A8_SSCALED);
                    STR(VK_FORMAT_B8G8R8A8_UINT);
                    STR(VK_FORMAT_B8G8R8A8_SINT);
                    STR(VK_FORMAT_B8G8R8A8_SRGB);
                    STR(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_UINT_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_SINT_PACK32);
                    STR(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_UINT_PACK32);
                    STR(VK_FORMAT_A2R10G10B10_SINT_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_UINT_PACK32);
                    STR(VK_FORMAT_A2B10G10R10_SINT_PACK32);
                    STR(VK_FORMAT_R16_UNORM);
                    STR(VK_FORMAT_R16_SNORM);
                    STR(VK_FORMAT_R16_USCALED);
                    STR(VK_FORMAT_R16_SSCALED);
                    STR(VK_FORMAT_R16_UINT);
                    STR(VK_FORMAT_R16_SINT);
                    STR(VK_FORMAT_R16_SFLOAT);
                    STR(VK_FORMAT_R16G16_UNORM);
                    STR(VK_FORMAT_R16G16_SNORM);
                    STR(VK_FORMAT_R16G16_USCALED);
                    STR(VK_FORMAT_R16G16_SSCALED);
                    STR(VK_FORMAT_R16G16_UINT);
                    STR(VK_FORMAT_R16G16_SINT);
                    STR(VK_FORMAT_R16G16_SFLOAT);
                    STR(VK_FORMAT_R16G16B16_UNORM);
                    STR(VK_FORMAT_R16G16B16_SNORM);
                    STR(VK_FORMAT_R16G16B16_USCALED);
                    STR(VK_FORMAT_R16G16B16_SSCALED);
                    STR(VK_FORMAT_R16G16B16_UINT);
                    STR(VK_FORMAT_R16G16B16_SINT);
                    STR(VK_FORMAT_R16G16B16_SFLOAT);
                    STR(VK_FORMAT_R16G16B16A16_UNORM);
                    STR(VK_FORMAT_R16G16B16A16_SNORM);
                    STR(VK_FORMAT_R16G16B16A16_USCALED);
                    STR(VK_FORMAT_R16G16B16A16_SSCALED);
                    STR(VK_FORMAT_R16G16B16A16_UINT);
                    STR(VK_FORMAT_R16G16B16A16_SINT);
                    STR(VK_FORMAT_R16G16B16A16_SFLOAT);
                    STR(VK_FORMAT_R32_UINT);
                    STR(VK_FORMAT_R32_SINT);
                    STR(VK_FORMAT_R32_SFLOAT);
                    STR(VK_FORMAT_R32G32_UINT);
                    STR(VK_FORMAT_R32G32_SINT);
                    STR(VK_FORMAT_R32G32_SFLOAT);
                    STR(VK_FORMAT_R32G32B32_UINT);
                    STR(VK_FORMAT_R32G32B32_SINT);
                    STR(VK_FORMAT_R32G32B32_SFLOAT);
                    STR(VK_FORMAT_R32G32B32A32_UINT);
                    STR(VK_FORMAT_R32G32B32A32_SINT);
                    STR(VK_FORMAT_R32G32B32A32_SFLOAT);
                    STR(VK_FORMAT_R64_UINT);
                    STR(VK_FORMAT_R64_SINT);
                    STR(VK_FORMAT_R64_SFLOAT);
                    STR(VK_FORMAT_R64G64_UINT);
                    STR(VK_FORMAT_R64G64_SINT);
                    STR(VK_FORMAT_R64G64_SFLOAT);
                    STR(VK_FORMAT_R64G64B64_UINT);
                    STR(VK_FORMAT_R64G64B64_SINT);
                    STR(VK_FORMAT_R64G64B64_SFLOAT);
                    STR(VK_FORMAT_R64G64B64A64_UINT);
                    STR(VK_FORMAT_R64G64B64A64_SINT);
                    STR(VK_FORMAT_R64G64B64A64_SFLOAT);
                    STR(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
                    STR(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
                    STR(VK_FORMAT_D16_UNORM);
                    STR(VK_FORMAT_X8_D24_UNORM_PACK32);
                    STR(VK_FORMAT_D32_SFLOAT);
                    STR(VK_FORMAT_S8_UINT);
                    STR(VK_FORMAT_D16_UNORM_S8_UINT);
                    STR(VK_FORMAT_D24_UNORM_S8_UINT);
                    STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
                    STR(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
                    STR(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
                    STR(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
                    STR(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
                    STR(VK_FORMAT_BC2_UNORM_BLOCK);
                    STR(VK_FORMAT_BC2_SRGB_BLOCK);
                    STR(VK_FORMAT_BC3_UNORM_BLOCK);
                    STR(VK_FORMAT_BC3_SRGB_BLOCK);
                    STR(VK_FORMAT_BC4_UNORM_BLOCK);
                    STR(VK_FORMAT_BC4_SNORM_BLOCK);
                    STR(VK_FORMAT_BC5_UNORM_BLOCK);
                    STR(VK_FORMAT_BC5_SNORM_BLOCK);
                    STR(VK_FORMAT_BC6H_UFLOAT_BLOCK);
                    STR(VK_FORMAT_BC6H_SFLOAT_BLOCK);
                    STR(VK_FORMAT_BC7_UNORM_BLOCK);
                    STR(VK_FORMAT_BC7_SRGB_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
                    STR(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
                    STR(VK_FORMAT_EAC_R11_UNORM_BLOCK);
                    STR(VK_FORMAT_EAC_R11_SNORM_BLOCK);
                    STR(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
                    STR(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
                    STR(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
                    STR(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
                    STR(VK_FORMAT_G8B8G8R8_422_UNORM);
                    STR(VK_FORMAT_B8G8R8G8_422_UNORM);
                    STR(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM);
                    STR(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
                    STR(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM);
                    STR(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM);
                    STR(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
                    STR(VK_FORMAT_R10X6_UNORM_PACK16);
                    STR(VK_FORMAT_R10X6G10X6_UNORM_2PACK16);
                    STR(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16);
                    STR(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
                    STR(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
                    STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
                    STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
                    STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
                    STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
                    STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
                    STR(VK_FORMAT_R12X4_UNORM_PACK16);
                    STR(VK_FORMAT_R12X4G12X4_UNORM_2PACK16);
                    STR(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16);
                    STR(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
                    STR(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
                    STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
                    STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
                    STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
                    STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
                    STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
                    STR(VK_FORMAT_G16B16G16R16_422_UNORM);
                    STR(VK_FORMAT_B16G16R16G16_422_UNORM);
                    STR(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM);
                    STR(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM);
                    STR(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM);
                    STR(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM);
                    STR(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);
                    STR(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM);
                    STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16);
                    STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16);
                    STR(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM);
                    STR(VK_FORMAT_A4R4G4B4_UNORM_PACK16);
                    STR(VK_FORMAT_A4B4G4R4_UNORM_PACK16);
                    STR(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK);
                    STR(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK);
                    STR(VK_FORMAT_A1B5G5R5_UNORM_PACK16);
                    STR(VK_FORMAT_A8_UNORM);
                    STR(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
                    STR(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
                    STR(VK_FORMAT_R16G16_SFIXED5_NV);
#undef STR
				default: return "unknown vk_format";
			}
		}
	}
}
