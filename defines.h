#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>

#define STBI_SIMD
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <vk_mem_alloc.h>
#include <imgui.h>  
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include <filesystem>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <cstdint>
#include <limits> 
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <time.h>
#include <thread>
#include <span>

//#include <fastnoise.h>
#include "assets/fastnoise/FastNoise.h"

#include "rect.h"
#include "sphere.h"
#include "math.h"
#include "stringbuilder.h"

#include "debug.h"
#include "initializers.h"


//
//  Settings 
// 
const int MAX_FRAMES_IN_FLIGHT = 2;

//const unsigned int LOD_LEVELS = 7;
//const std::array<float, LOD_LEVELS - 1> LOD_DISTANCES = { 30, 40, 60, 100, 140, 180 };
//const std::array<float, LOD_LEVELS - 1> LOD_THRESHOLDS = { 0.5f, 0.5f, 0.5f, 0.4f, 0.3f, 0.1f };
//const std::array<float, LOD_LEVELS - 1> LOD_TARGET_ERROR = { 0.001f, 0.001f, 0.01f, 0.01f, 0.01f, 0.001f };

const unsigned int LOD_LEVELS = 4;
const std::array<float, LOD_LEVELS - 1> LOD_DISTANCES = { 200, 300, 400};
const std::array<float, LOD_LEVELS - 1> LOD_THRESHOLDS = { 0.5f, 0.5f, 0.1f };
const std::array<float, LOD_LEVELS - 1> LOD_TARGET_ERROR = { 0.1f, 0.4f, 0.6f};

#ifdef NDEBUG
//const char* const MODEL_PATH = "models/Paving_Stone_1_HIGH_UV+.obj";
const char* const MODEL_PATH = "models/Paving_Stone_1_LOW_UV+.obj";
#else
const char* const MODEL_PATH = "models/Paving_Stone_1_LOW_UV+.obj";
#endif

const std::string APPLICATION_NAME = "VulkanEngine";

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME };

const char* const VERTEX_SHADER = "compiled shaders/vert.spv";
const char* const FRAGMENT_SHADER = "compiled shaders/frag.spv";

const char* const PHONG_VERTEX_SHADER = "compiled shaders/phong.vert.spv";
const char* const PHONG_FRAGMENT_SHADER = "compiled shaders/phong.frag.spv";

const char* const WIREFRAME_VERT_SHADER = "compiled shaders/wireframe.vert.spv";
const char* const WIREFRAME_FRAG_SHADER = "compiled shaders/wireframe.frag.spv";

const char* const SUN_VERTEX_SHADER = "compiled shaders/sun.vert.spv";
const char* const SUN_FRAGMENT_SHADER = "compiled shaders/sun.frag.spv";

const char* const PBR_VERTEX_SHADER = "compiled shaders/pbr.vert.spv";
const char* const PBR_FRAGMENT_SHADER = "compiled shaders/pbr.frag.spv";

const char* const ALBEDO_TEXTURE = "textures/Paving_Stone_1_Diffuse.jpg";
const char* const NORMAL_TEXTURE = "textures/Paving_Stone_1_Normal.jpg";

//
//  MACRO SETUP 
// 

#define VEC2 glm::vec2
#define VEC3 glm::vec3
#define VEC4 glm::vec4
#define MAT3 glm::mat3
#define MAT4 glm::mat4
#define QUAT glm::quat
#define COLOR VEC4

#define VERTEX Vertex

#define BBOX BBox
#define BYTE uint8_t
#define UINT uint32_t 
#define FLOAT float
#define SIZE size_t

#define IVEC2 glm::ivec2
#define IVEC3 glm::ivec3
#define IVEC4 glm::ivec4

#define UVEC2 glm::uvec2
#define UVEC3 glm::uvec3
#define UVEC4 glm::uvec4

#define VECT2 glm::vec<2, T> 
#define VECT3 glm::vec<3, T> 
#define VECT4 glm::vec<4, T> 

#define MIN glm::min
#define MAX glm::max
#define ABS glm::abs
#define DISTANCE glm::distance

#define DOT glm::dot
#define CROSS glm::cross
#define INF std::numeric_limits<FLOAT>::infinity()
#define PI glm::pi<float>()
#define RAD glm::radians
#define NORM glm::normalize

#define MAP std::map
#define VECTOR std::vector
#define PACKED_VERTEX PackedVertex
#define QUANTIZED_VERTEX QuantizedVertex
#define VERTEX Vertex

#define ct_position       (1 << 0)
#define ct_rotation       (1 << 1)
#define ct_scale          (1 << 2)
#define ct_color          (1 << 3)
#define ct_boundingBox    (1 << 4)
#define ct_linearVelocity (1 << 5)
#define ct_radialVelocity (1 << 6)
#define ct_chunk          (1 << 7)
#define ct_chunk_id       (1 << 8)
#define ct_mesh_id        (1 << 9)
#define ct_material_id    (1 << 10)
#define ct_render_index   (1 << 11) // translation table from renderindex to entityid

#define ct_created        (1 << 31) // only set on entity if its new 

#define CHUNK_SIZE_X 16  	// 16x512x16 = 131072 / 128kb
#define CHUNK_SIZE_Y 256    // = x24 = 6291456 vertices max
#define CHUNK_SIZE_Z 16
#define CHUNK_BLOCK_COUNT       (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)
#define CHUNK_MAX_VERTICE_COUNT (CHUNK_BLOCK_COUNT * 24)
#define CHUNK_SIZE_XZ           (CHUNK_SIZE_X * CHUNK_SIZE_Z)
#define CHUNK_SIZE_XYZ          (CHUNK_SIZE_X * CHUNK_SIZE_Z * CHUNK_SIZE_Y)


#define WINDOW ui::Window 
#define WINDOW_ID WindowId 
#define ENTITY_ID EntityId 

#ifdef NDEBUG

    const bool VERBOSE = false;
    #define DEBUG(msg, ...) ;
    #define DEBUG_IMAGE(image) ;
    #define DEBUG_BUFFER(buffer) ;

#else
    
    const bool VERBOSE = true;

    #define DEBUG(msg, ...) printf((msg), ##__VA_ARGS__); 
    #define DEBUG_IMAGE(image)                                                                      \
    {                                                                                               \
        if(image.name != nullptr)                                                                   \
        {                                                                                           \
            DEBUG("#  Image info: '%s' \n", image.name)                                             \
        }                                                                                           \
        DEBUG("#      extent: %d x %d \n", image.width, image.height)                               \
        DEBUG("#       depth: %d \n", image.depth)                                                  \
        DEBUG("#   miplevels: %d \n", image.mipmapLevels)                                           \
        DEBUG("#      layout: %s \n", vkengine::debug::imageLayoutString(image.layout).c_str())     \
        DEBUG("#      format: %s \n", vkengine::debug::formatString(image.format).c_str())          \
    }
    #define DEBUG_BUFFER(buffer)                                                                    \
    {                                                                                               \
        if(buffer.name != nullptr)                                                                  \
        {                                                                                           \
            DEBUG("#  Buffer info: '%s'\n", buffer.name)                                            \
        }                                                                                           \
        DEBUG("#         size: %d\n", buffer.info.size)                                             \
    }

#endif


#define BENCHMARK 
#ifdef BENCHMARK 
    #define START_TIMER auto __timer_started = std::chrono::high_resolution_clock::now();
    #define END_TIMER(__msg, ...)                                                           \
    {                                                                                       \
        auto __timer_elapsed = std::chrono::high_resolution_clock::now() - __timer_started; \
        auto __ms = std::chrono::nanoseconds(__timer_elapsed).count() / 1000000.0f;         \
        printf(__msg, ##__VA_ARGS__);                                                       \
        printf("%.6f ms\n", __ms);                                                          \
    }
#else
    #define START_TIMER ;
    #define END_TIMER(name) ;
#endif 


#define VK_CHECK(f)																			         	\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vkengine::debug::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
} 

#define BEGIN_COMMAND_BUFFER VkCommandBuffer commandBuffer = beginCommandBuffer(); 
#define END_COMMAND_BUFFER if(commandBuffer != VK_NULL_HANDLE) { endCommandBuffer(commandBuffer); commandBuffer = VK_NULL_HANDLE; }

#define DESTROY_BUFFER(__vma_allocator, __buffer)                                       \
{                                                                                       \
    if(__vma_allocator != VK_NULL_HANDLE && __buffer.alloc != VK_NULL_HANDLE)           \
    {                                                                                   \
        vmaDestroyBuffer(__vma_allocator, __buffer.buffer, __buffer.alloc);             \
    }                                                                                   \
    __buffer.alloc = VK_NULL_HANDLE;                                                    \
    __buffer.buffer = VK_NULL_HANDLE;                                                   \
}

#define DESTROY_BUFFERS(__vma_allocator, __buffers)                                     \
    for(int __i = 0; __i < __buffers.size(); __i++) DESTROY_BUFFER(__vma_allocator, __buffers[__i]);  

#define DESTROY_IMAGE(__vma_allocator, __image)                                         \
    if(__vma_allocator != VK_NULL_HANDLE && __image.allocation != VK_NULL_HANDLE)       \
    {                                                                                   \
        vmaDestroyImage(__vma_allocator, __image.img, __image.allocation);              \
    }                                                                                   \
    __image.allocation = VK_NULL_HANDLE;                                                \
    __image.img = VK_NULL_HANDLE;                                                       \
    __image.view = VK_NULL_HANDLE;                                                      \
    __image.sampler = VK_NULL_HANDLE;                                                   


#ifndef NDEBUG                                                  
    #define SET_ALLOCATION_NAME(allocator, allocation, name)            \
        if (name != nullptr)                                            \
        {                                                               \
            vmaSetAllocationName(allocator, allocation, name);          \
        }                                                         
    #define SET_BUFFER_NAME(allocator, buffer, _name)                   \
        if(_name != nullptr)                                            \
        {                                                               \
            vmaSetAllocationName(allocator, buffer.alloc, _name);       \
            buffer.name = _name;                                        \
        }
#else
    #define SET_ALLOCATION_NAME(allocator, allocation, name)    ;
    #define SET_BUFFER_NAME(allocator, buffer, name)            ; 
#endif 



//
//  Local Includes 
//
#include "io.h"
#include "tools.h"
#include "buffer.h"
#include "image.h"
#include "vertex.h"
#include "aabb.h"
#include "octree.h"
#include "octree-adaptor.h"
#include "frustum.h"
#include "input.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "material.h"
#include "mesh.h"
#include "model.h"
#include "heightmap.h"
#include "assets.h"
#include "configuration.h"
#include "device.h"
#include "entity.h"
#include "scene.h"
#include "render.h"
#include "grid.h"
#include "ui.h"
#include "textoverlay.h"
#include "engine.h"

#include "debugwindows.h"

