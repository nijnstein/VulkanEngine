#include "defines.h"
#include <meshoptimizer.h>

namespace vkengine
{
	void MeshInfo::quantize(bool removeVertices)
	{
		quantized.clear();
		quantized.resize(vertices.size());

		int i = 0;
		while (i < vertices.size() - 4)
		{
			quantized[i + 0] = vertices[i + 0].quantize();
			quantized[i + 1] = vertices[i + 1].quantize();
			quantized[i + 2] = vertices[i + 2].quantize();
			quantized[i + 3] = vertices[i + 3].quantize();
			i += 4;
		} 
		for (; i < vertices.size(); i++)
		{
			quantized[i] = vertices[i].quantize();
		}

		if (removeVertices)
		{
			vertices.clear();
		}
	}

	AABB MeshInfo::calculateAABB()
	{
		aabb.FromVertices(vertices.data(), vertices.size());
		return aabb;
	}

	VEC3 MeshInfo::calculateCentroid()
	{
		VEC3 centroid = VEC3(0);
		for (int i = 0; i < vertices.size(); i++)
		{
			centroid += vertices[i].pos();
		}
		return centroid / (float)vertices.size();
	}

	void MeshInfo::scaleVertices(float scale)
	{
		scaleVertices(VEC3(scale));
	}

	void MeshInfo::scaleVertices(VEC3 scale)
	{
		for (int i = 0; i < vertices.size(); i++)
		{
			vertices[i].posAndValue = VEC4(vertices[i].pos() * scale, vertices[i].posAndValue.w);
		}
		aabb = calculateAABB();
	}

	void MeshInfo::scaleVertices(VEC3 scale, int from, int count)
	{
		for (int i = from; i < from + count; i++)
		{
			vertices[i].posAndValue = VEC4(vertices[i].pos() * scale, vertices[i].posAndValue.w);
		}
		aabb = calculateAABB();
	}


	void MeshInfo::translateVertices(VEC3 translation)
	{
		for (int i = 0; i < vertices.size(); i++)
		{
			vertices[i].posAndValue.x = vertices[i].posAndValue.x + translation.x;
			vertices[i].posAndValue.y = vertices[i].posAndValue.y + translation.y;
			vertices[i].posAndValue.z = vertices[i].posAndValue.z + translation.z;
		}
		aabb = calculateAABB();
	}

	VEC3 MeshInfo::computeFaceNormal(VEC3 p1, VEC3 p2, VEC3 p3)
	{
		// Uses p2 as a new origin for p1,p3
		auto a = p3 - p2;
		auto b = p1 - p2;

		// Compute the cross product a X b to get the face normal
		return /*glm::normalize(*/glm::cross(a, b);//);
	}

	void MeshInfo::computeVertexNormals()
	{
		//
		// 1> compute normal foreach face
		// 2> add up face normal to each vertex normal touching the face
		// 3> normalize vertex normals 
		//
		for (SIZE i = 0; i < vertices.size(); i++)
		{
			vertices[i].setNormal(VEC3(0));
		}

		for (SIZE i = 0; i < indices.size(); i += 3)
		{
			VEC3 p1 = vertices[indices[i]].pos();
			VEC3 p2 = vertices[indices[i + 1LL]].pos();
			VEC3 p3 = vertices[indices[i + 2LL]].pos();

			auto faceNormal = computeFaceNormal(p1, p2, p3);

			// add the face normal to the 3 vertices normal touching this face
			vertices[indices[i]].setNormal(vertices[indices[i]].normal() + faceNormal);
			vertices[indices[i + 1LL]].setNormal(vertices[indices[i + 1LL]].normal() + faceNormal);
			vertices[indices[i + 2LL]].setNormal(vertices[indices[i + 2LL]].normal() + faceNormal);
		}

		// normalize vertices normal
		for (uint32_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].setNormal(glm::normalize(vertices[i].normal()));
		}
	}

	void MeshInfo::setVertexColors(VEC4 rgba)
	{
		for (auto& v : vertices)
		{
			v.setColor(VEC3(rgba));
		}
	}

	void MeshInfo::clearNormals()
	{
		for (int iv = 0; iv < vertices.size(); iv++)
		{
			vertices[iv].setNormal(VEC3(0, 0, 0));
		}
	}

	// only use this on a mesh with lod0 only!
	void MeshInfo::removeDuplicateVertices()
	{
		std::vector<PACKED_VERTEX> v;
		std::vector<UINT> i;

		v.reserve(vertices.size());
		i.reserve(indices.size());

		for (int iv = 0; iv < vertices.size(); iv++)
		{
			PACKED_VERTEX vertex = vertices[iv];
			bool found = false;

			for (int j = 0; j < v.size() && !found; j++)
			{
				PACKED_VERTEX other = v[j];
				if (vertex.posAndValue == other.posAndValue
					&&
					vertex.colorAndNormal == other.colorAndNormal
					&&
					vertex.uvAndNormal == other.uvAndNormal)
				{
					i.push_back(j);
					found = true;
				}
			}

			if (!found)
			{
				i.push_back(v.size());
				v.push_back(vertex);
			}
		}

		if (lods.size() > 0)
		{
			auto offset = &lods[0];
			offset->indexCount = i.size();
			offset->indexOffset = 0;
		}

		vertices.clear();
		for (int j = 0; j < v.size(); j++) vertices.push_back(v[j]);

		indices.clear();
		for (int j = 0; j < i.size(); j++) indices.push_back(i[j]);
	}

	void MeshInfo::optimizeMesh()
	{
		int lodLevels = lodDistances.size();
		if (lodLevels == 0)
		{
			unsigned int* indices = this->indices.data();
			unsigned int indexCount = this->indices.size();
			unsigned int vertexCount = this->vertices.size();
			PACKED_VERTEX* vertices = this->vertices.data(); // xyz == first member 

			meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
			meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
			meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));

			return;
		}

		// define lod level 0 
		MeshLODLevelInfo lod0;
		lod0.meshId = meshId;
		lod0.indexCount = indices.size();
		lod0.indexOffset = 0;
		lod0.lodLevel = 0;

		lods.clear();
		lods.push_back(lod0);

		// no use running this on very small meshes
		if (indices.size() > 48)
		{
			unsigned int* indices = this->indices.data();
			unsigned int indexCount = this->indices.size();
			unsigned int vertexCount = this->vertices.size();
			PACKED_VERTEX* vertices = this->vertices.data(); // xyz == first member 

			// optimize the LOD0 mesh 
			meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
			meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
			meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));
			std::vector<std::vector<unsigned int>> lods = { };

			unsigned int totalIndexCount = indexCount;
			bool noLod = false;

			for (int i = 1; i < lodLevels + 1; i++)
			{
				DEBUG("   computing lod %d", i);

				float threshold = lodThresholds[i - 1];
				size_t target_index_count = size_t(indexCount * threshold);
				float target_error = lodTargetErrors[i - 1];

				std::vector<unsigned int> lodIndices(indexCount);
				float lod_error = 0.f;

				unsigned int lodIndexCount;

				if (lodSimplifySloppy[i - 1])
				{
					lodIndexCount = meshopt_simplifySloppy(
						&lodIndices[0],
						indices,
						indexCount,
						&vertices[0].posAndValue.x,
						vertexCount,
						sizeof(PACKED_VERTEX),
						target_index_count,
						target_error,
						&lod_error);
				}
				else
				{
					lodIndexCount = meshopt_simplify(
						&lodIndices[0],
						indices,
						indexCount,
						&vertices[0].posAndValue.x,
						vertexCount,
						sizeof(PACKED_VERTEX),
						target_index_count,
						target_error,
						 meshopt_SimplifyLockBorder,
						&lod_error);
				}

				if (lodIndexCount == 0 || lodIndexCount > indexCount * 0.95)
				{
					// lod not effective, cancel out 
					noLod = i == 1;
					DEBUG("   FAIL\n", i);
					break;
				}
				else
				{
					DEBUG("   OK, original indexcount = %d, lod %d indexcount = %d\n", indexCount, i, lodIndexCount);
				}

				lodIndices.resize(lodIndexCount);

				meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndexCount, vertexCount);
				meshopt_optimizeOverdraw(lodIndices.data(), lodIndices.data(), lodIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
				lods.push_back(lodIndices);


				// lod info (filled in below)
				MeshLODLevelInfo lodn;
				lodn.meshId = meshId;
				lodn.indexCount = lodIndexCount;
				lodn.indexOffset = 0;
				lodn.lodLevel = i;
				this->lods.push_back(lodn);

				// next lod level 
				totalIndexCount += lodIndexCount;
				indexCount = lodIndexCount;
				indices = lods[lods.size() - 1].data();
			}
			if (!noLod)
			{
				// rebuild index buffer starting with the smallest lod 
				std::vector<unsigned int> indexbuffer{};
				indexbuffer.reserve(totalIndexCount);

				for (int i = this->lods.size() - 1; i >= 1; i--)
				{
					for (int j = 0; j < lods[i - 1].size(); j++)
					{
						indexbuffer.push_back(lods[i - 1][j]);
					}
				}
				for (int j = 0; j < this->indices.size(); j++)
				{
					indexbuffer.push_back(this->indices[j]);
				}

				// index lot levels in mesh lod info 
				int offset = 0;
				for (int i = this->lods.size() - 1; i >= 1; i--)
				{
					MeshLODLevelInfo& lodn = this->lods[i];
					lodn.indexOffset = offset;

					unsigned int size = lods[i - 1].size();
					lodn.indexCount = size;
					offset += size;
				}
				this->lods[0].indexCount = this->indices.size();
				this->lods[0].indexOffset = offset;

				// optimize mesh, with lowest lod first ensuring most efficient use on gpu
				meshopt_optimizeVertexFetch(vertices, indexbuffer.data(), totalIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));

				// finally set the generated indices to the mesh 
				this->indices = indexbuffer;

				// recompute normals
				// computeVertexNormals(model);
			}
		}
	}
}