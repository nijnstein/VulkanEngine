#pragma once

namespace vkengine
{
	namespace assets
	{
		enum class AssetType
		{
			VKE_ASSET_DATA = 0,
			VKE_ASSET_TEXTURE = 1,
			VKE_ASSET_SHADER = 2
		};

		static int nextMeshId()
		{
			static int meshIdGenerator = 0;
			return meshIdGenerator++;
		}

		VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code);
		ModelData loadObj(const char* path, Material* material, float scale = 1.0f, bool computeNormals = true, bool removeDuplicateVertices = true, bool absoluteScaling = false, bool computeTangents = true, bool calcLods = true, bool optimize = true);
		

		glm::vec3 computeFaceNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
		void computeVertexNormals(ModelData& model);
		void computeTangentsFromVerticesAndUV(MeshInfo& mesh);
		void computeTangentsFromVerticesAndUV(ModelData& model);
		ModelData removeVertexDuplicates(ModelData model);

		void optimizeMesh(MeshInfo& mesh, bool calculateLOD);
	}
}
