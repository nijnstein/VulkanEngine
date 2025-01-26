#pragma once

namespace vkengine
{
	typedef int32_t MeshId;

	struct MeshLODLevelInfo
	{
		MeshId meshId; 
		UINT lodLevel; 
		UINT indexOffset; 
		UINT indexCount; 
	};

	struct MeshInfo;

	typedef void (*RequestMeshCallback)(void* engine, MeshInfo* mesh, void* userdata);

	struct MeshInfo
	{
		std::string name;

		MeshId meshId{ -1 };
		MaterialId materialId{ -1 };

		// in model space
		AABB aabb;
		UINT cullDistance{0}; 

		std::vector<PACKED_VERTEX> vertices;
		std::vector<QUANTIZED_VERTEX> quantized;  
		std::vector<UINT> indices;
		std::vector<MeshLODLevelInfo> lods; 

		std::vector<float> lodDistances = { 200, 300, 400, 500 };
		std::vector<float> lodThresholds = { 0.5f, 0.5f, 0.2f, 0.2f };
		std::vector<float> lodTargetErrors = { 0.1f, 0.4f, 0.6f, 0.7f };
		std::vector<bool> lodSimplifySloppy = { false, false, true, true };

		void* userdataPtr; 
		RequestMeshCallback requestMesh;

		inline bool isLoaded() const {
			return indices.size() > 0 && (vertices.size() > 0 || quantized.size() > 0);
		}

		inline bool isQuantized() const {
			return quantized.size() > 0;
		}

		void quantize(bool removeVertices = true);
		AABB calculateAABB();
		VEC3 calculateCentroid();
		void scaleVertices(float scale);
		void scaleVertices(VEC3 scale);
		void scaleVertices(VEC3 scale, int from, int count);
		void translateVertices(VEC3 translation);
		void removeDuplicateVertices();// only checks lod0 !!!
		inline VEC3 computeFaceNormal(VEC3 p1, VEC3 p2, VEC3 p3);
		void computeVertexNormals();
		void setVertexColors(VEC4 rgba);
		void clearNormals();
		void optimizeMesh();
	};
}
