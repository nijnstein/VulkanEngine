#pragma once

namespace vkengine
{
	struct SceneInfoBufferObject
	{
		alignas(16) MAT4 view;
		alignas(16) MAT4 proj;
		alignas(16) MAT4 projectionView; // view * projection

		alignas(16) MAT4 normal;
		alignas(16) VEC3 lightPosition;
		alignas(16) VEC3 viewPosition;

		// directional light 
		alignas(16) VEC3 lightDirection;
		alignas(16) VEC3 lightColor; 
		alignas(16) float lightIntensity; 
	};
}