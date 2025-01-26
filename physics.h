#pragma once

namespace vkengine
{
	const BYTE BOX_COLLIDER = 0;
	const BYTE SPHERE_COLLIDER = 1;	// only use sphere colliders on static objects!!
	const BYTE MESH_COLLIDER = 2;
	const BYTE MESH_SPHERE_COLLIDER = 3;
	const BYTE MESH_BBOX_COLLIDER = 4;

	struct RayCastResult
	{
		VEC3 intersection;
		EntityId entityId;
		int triangleIndex;
	};

	struct Ray3d
	{
		VEC3 origin{};
		VEC3 direction{};
	}; 

	struct Collider
	{
		// sphere / box / mesh 
		BYTE colliderType;

		// alignment to 4 byte boundary / doubles as mesh id  
		BYTE r1;
		BYTE r2;
		BYTE r3;
								 		
		inline static Collider fromMesh(MeshId meshId)
		{
			assert(meshId >= 0);
			return {
				.colliderType = MESH_BBOX_COLLIDER,	// data for bounds can be found in ct_boundingBox
				.r1 = (BYTE)(meshId & 0x000000FF),
				.r2 = (BYTE)(meshId & 0x0000FF00),
				.r3 = (BYTE)(meshId & 0x00FF0000),
			};
		}						 	   

		inline MeshId getMeshId()
		{
			return r1 | (r2 << 8) | (r3 << 16); 
		}
	};

	namespace math
	{
		struct TriangleIntersection
		{
			VEC3 point;
			VEC3 uvi;
			bool intersects;
		};

		inline static TriangleIntersection intersectTriangle(VEC3 rayOrigin, VEC3 rayDir, VEC3 A, VEC3 B, VEC3 C);
		inline static FLOAT Det2(FLOAT x1, FLOAT x2, FLOAT y1, FLOAT y2);
		inline static bool intersectLine(VEC2 v1, VEC2 v2, VEC2 v3, VEC2 v4, VEC2& intersection);
		inline static bool intersectPlane(VEC3 rayDirection, VEC3 rayOrigin, VEC3 planeNormal, VEC3 coord, VEC3& intersection);

		
		inline static bool insideRotatedBox(VEC3 point, VEC3 center, VEC3 halfSize, VEC3 normal /* normals perpendicular to planes of box */)
		{
			FLOAT r = ABS(DOT(point - center, normal));

			bool inside =
				r <= halfSize.x &&
				r <= halfSize.y &&
				r <= halfSize.z;

			return inside;
		}

		inline static bool insideOrOnBoxNotRotated(BBOX box, VEC3 point)
		{
			return
				((point.x >= box.min.x) & (point.y >= box.min.y) & (point.z >= box.min.z)
				&
				(point.x <= box.max.x) & (point.y <= box.max.y) & (point.z <= box.max.z)) != 0;
		}	
		
		inline static bool insideBoxNotRotated(BBOX box, VEC3 point)
		{
			return
				((point.x > box.min.x) & (point.y > box.min.y) & (point.z > box.min.z)
				&
				(point.x < box.max.x) & (point.y < box.max.y) & (point.z < box.max.z)) != 0;
		}

		inline static bool intersectBoxNotRotated(BBOX box, Ray3d ray, VEC3& intersection)
		{

			return false; 
		}

	}
}