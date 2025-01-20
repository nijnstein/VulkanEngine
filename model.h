#pragma once

namespace vkengine
{
	struct ModelData
	{
		std::vector<MeshInfo> meshes;
		AABB aabb;

		AABB calculateAABB()
		{
			AABB aabb;

			for (int i = 0; i < meshes.size(); i++)
			{
				AABB b{};

				b.FromVertices(meshes[i].vertices.data(), meshes[i].vertices.size());

				aabb.min = MIN(aabb.min, b.min);
				aabb.max = MAX(aabb.max, b.max);
			}

			aabb.FromMinMax();
			return aabb;
		}
	};

	typedef int32_t ModelId;

	struct ModelInfo
	{
		ModelId modelId;
		AABB aabb;

		std::vector<MeshId> meshes;
		std::vector<MaterialId> materialIds;
	};
}
