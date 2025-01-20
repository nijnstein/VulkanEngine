#include "defines.h"
#include <meshoptimizer.h>


namespace vkengine
{
	namespace assets
	{
		VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
			return shaderModule;
		}

		glm::vec3 computeFaceNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
		{
			// Uses p2 as a new origin for p1,p3
			auto a = p3 - p2;
			auto b = p1 - p2;

			// Compute the cross product a X b to get the face normal
			return /*glm::normalize(*/glm::cross(a, b);//);
		}

		void computeVertexNormals(ModelData& model)
		{
			//
			// 1> compute normal foreach face
			// 2> add up face normal to each vertex normal touching the face
			// 3> normalize vertex normals 
			//
			for (int j = 0; j < model.meshes.size(); j++)
			{
				for (uint32_t i = 0; i < model.meshes[j].indices.size(); i += 3)
				{
					glm::vec3 p1 = model.meshes[j].vertices[model.meshes[j].indices[i]].pos();
					glm::vec3 p2 = model.meshes[j].vertices[model.meshes[j].indices[i + 1LL]].pos();
					glm::vec3 p3 = model.meshes[j].vertices[model.meshes[j].indices[i + 2LL]].pos();

					auto faceNormal = computeFaceNormal(p1, p2, p3);

					// add the face normal to the 3 vertices normal touching this face
					model.meshes[j].vertices[model.meshes[j].indices[i]].addToNormal(faceNormal);
					model.meshes[j].vertices[model.meshes[j].indices[i + 1LL]].addToNormal(faceNormal);
					model.meshes[j].vertices[model.meshes[j].indices[i + 2LL]].addToNormal(faceNormal);
				}

				// normalize vertices normal
				for (uint32_t i = 0; i < model.meshes[j].vertices.size(); i++)
				{
					model.meshes[j].vertices[i].setNormal(glm::normalize(model.meshes[j].vertices[i].normal()));
				}
			}
		}

		void computeTangentsFromVerticesAndUV(MeshInfo& mesh)
		{
			for (int i = 0; i < mesh.indices.size(); i += 3)
			{
				UINT i0 = mesh.indices[i + 0];
				UINT i1 = mesh.indices[i + 1];
				UINT i2 = mesh.indices[i + 2];

				PACKED_VERTEX& v0 = mesh.vertices[i0];
				PACKED_VERTEX& v1 = mesh.vertices[i1];
				PACKED_VERTEX& v2 = mesh.vertices[i2];

				glm::vec2 duv1 = v1.uv() - v0.uv();
				glm::vec2 duv2 = v2.uv() - v0.uv();

				float k = 1 / (duv1.x * duv2.y - duv2.x * duv1.y);
				
				glm::mat2x2 UV(duv2.y, -duv1.y, -duv2.x, duv1.x);
				glm::mat2x3 E(v1.pos() - v0.pos(), v2.pos() - v0.pos());
				glm::mat2x3 TB = k * E * UV;

		//		v0.tangent += TB[0];
		//		v0.bitangent += TB[1];
		//		v1.tangent += TB[0];
		//		v1.bitangent += TB[1];
		//		v2.tangent += TB[0];
		//		v2.bitangent += TB[1];
			}
		}

		void computeTangentsFromVerticesAndUV(ModelData& model)
		{
			for (auto& mesh : model.meshes) computeTangentsFromVerticesAndUV(mesh);
		}

		ModelData removeVertexDuplicates(ModelData model)
		{
			ModelData output;

			std::unordered_map<PACKED_VERTEX, uint32_t> unique{};

			for (int j = 0; j < model.meshes.size(); j++)
			{
				auto mesh = model.meshes[j];
				MeshInfo outputMesh{};
				outputMesh.meshId = mesh.meshId;
				outputMesh.materialId = mesh.materialId;

				for (const auto& vertex : mesh.vertices)
				{
					if (unique.count(vertex) == 0)
					{
						unique[vertex] = static_cast<uint32_t>(outputMesh.vertices.size());
						outputMesh.vertices.push_back(vertex);
					}

					outputMesh.indices.push_back(unique[vertex]);
				}

				output.meshes.push_back(outputMesh);
				unique.clear(); 
			}

			return output;
		}

		ModelData loadObj(const char* path, Material* material, float scale, bool computeNormals, bool removeDuplicateVertices, bool absoluteScaling, bool computeTangents, bool calcLods, bool optimize)
		{
			DEBUG("LOADING OBJECT: %s\n", path);
			START_TIMER

			ModelData model;

			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::vector<glm::vec3> computedNormals;
			std::string warn, err;

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
			{
				throw std::runtime_error(warn + err);
			}

			uint32_t vertexIndex = 0;

			bool hasNormals = attrib.normals.size() > 0;
			bool hasUV = attrib.texcoords.size() > 0;

			for (const auto& shape : shapes)
			{
				MeshInfo mesh{};
				mesh.meshId = nextMeshId();
				mesh.materialId = material == nullptr ? -1 : material->materialId;
				mesh.name = shape.name;

				int i = 0;
				for (const auto& index : shape.mesh.indices)
				{
					PACKED_VERTEX vertex{};

					vertex.posAndValue = 
					{
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
						0
					};

					vertex.colorAndNormal = { 1.0f, 1.0f, 1.0f, 0.0f };

					if (hasUV && hasNormals)
					{
						vertex.uvAndNormal =
						{
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
						    attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};

						vertex.colorAndNormal.w = attrib.normals[3 * index.normal_index + 0];
					}
					else
					if (hasUV)
					{
						vertex.uvAndNormal = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
							0,
							0
						};
					}
					else
					if (hasNormals)
					{
						vertex.colorAndNormal.w = attrib.normals[3 * index.normal_index + 0];
					    vertex.uvAndNormal.z = attrib.normals[3 * index.normal_index + 1];
					    vertex.uvAndNormal.w = attrib.normals[3 * index.normal_index + 2];						
					}

					mesh.vertices.push_back(vertex);
					mesh.indices.push_back(vertexIndex);
					vertexIndex++;
				}

				model.meshes.push_back(mesh);
			}

			if (absoluteScaling)
			{
				AABB aabb = model.calculateAABB();
				
				glm::vec3 s = glm::vec3(scale, scale, scale) / fmaxf(fmaxf(aabb.size.x, aabb.size.y), aabb.size.z);

				for (int j = 0; j < model.meshes.size(); j++)
				{
					model.meshes[j].scaleVertices(s); 
				}
				model.aabb = model.calculateAABB();
			}
			else
			{
				for (int j = 0; j < model.meshes.size(); j++)
				{
					model.meshes[j].scaleVertices(scale);
				}
				model.aabb = model.calculateAABB();
			}

			if (removeDuplicateVertices)
			{
				model = removeVertexDuplicates(model);
			}

			if (computeNormals && !hasNormals)
			{
				DEBUG("   computing normals\n");
				computeVertexNormals(model);
				hasNormals = true;
			}

			if (computeTangents && hasUV && hasNormals)
			{
				//DEBUG("   computing tangents\n");
				//computeTangentsFromVerticesAndUV(model);
			}

			// define lods 
			if (calcLods)
			{
				for (auto& mesh : model.meshes)
				{
					// define lod level 0 
					MeshLODLevelInfo lod0;
					lod0.meshId = mesh.meshId;
					lod0.indexCount = mesh.indices.size();
					lod0.indexOffset = 0;
					lod0.lodLevel = 0;
					mesh.lods.push_back(lod0);

					// no use running this on very small meshes
					if (mesh.indices.size() > 100)
					{
						unsigned int* indices = mesh.indices.data();
						unsigned int indexCount = mesh.indices.size();
						unsigned int vertexCount = mesh.vertices.size();
						PACKED_VERTEX* vertices = mesh.vertices.data(); // xyz == first member 

						// optimize the LOD0 mesh 
						if (optimize)
						{
							DEBUG("   optimizing mesh\n");
							meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
							meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue[0], vertexCount, sizeof(PACKED_VERTEX), 1.05f);
							meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue, vertexCount, sizeof(PACKED_VERTEX));
						}
						std::vector<std::vector<unsigned int>> lods = { };

						unsigned int totalIndexCount = indexCount;

						for (int i = 1; i < LOD_LEVELS; i++)
						{
							DEBUG("   computing lod %d\n", i);

							float threshold = LOD_THRESHOLDS[i - 1];
							size_t target_index_count = size_t(indexCount * threshold);
							float target_error = LOD_TARGET_ERROR[i - 1];

							std::vector<unsigned int> lodIndices(indexCount);
							float lod_error = 0.f;

							unsigned int lodIndexCount = meshopt_simplify(
								&lodIndices[0],
								indices,
								indexCount,
								&vertices[0].posAndValue.x,
								vertexCount,
								sizeof(PACKED_VERTEX),
								target_index_count,
								target_error,
								/* options= */ 0,
								&lod_error);

							lodIndices.resize(lodIndexCount);

							if (optimize)
							{
								meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndexCount, vertexCount);
								meshopt_optimizeOverdraw(lodIndices.data(), lodIndices.data(), lodIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
							}
							lods.push_back(lodIndices);

							totalIndexCount += lodIndexCount;

							// lod info (filled in below)
							MeshLODLevelInfo lodn;
							lodn.meshId = mesh.meshId;
							lodn.indexCount = lodIndexCount;
							lodn.indexOffset = 0;
							lodn.lodLevel = i;
							mesh.lods.push_back(lodn);

							// next lod level 
							indexCount = lodIndexCount;
							indices = lods[lods.size() - 1].data();
						}

						// rebuild index buffer starting with the smallest lod 
						std::vector<unsigned int> indexbuffer{};
						indexbuffer.reserve(totalIndexCount);

						for (int i = LOD_LEVELS - 1; i >= 1; i--)
						{
							for (int j = 0; j < lods[i - 1].size(); j++)
							{
								indexbuffer.push_back(lods[i - 1][j]);
							}
						}
						for (int j = 0; j < mesh.indices.size(); j++)
						{
							indexbuffer.push_back(mesh.indices[j]);
						}

						// index lot levels in mesh lod info 
						int offset = 0;
						for (int i = LOD_LEVELS - 1; i >= 1; i--)
						{
							MeshLODLevelInfo& lodn = mesh.lods[i];
							lodn.indexOffset = offset;

							unsigned int size = lods[i - 1].size();
							lodn.indexCount = size;
							offset += size;
						}
						mesh.lods[0].indexCount = mesh.indices.size();
						mesh.lods[0].indexOffset = offset;

						// optimize mesh, with lowest lod first ensuring most efficient use on gpu
						if (optimize)
							meshopt_optimizeVertexFetch(vertices, indexbuffer.data(), totalIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));

						// finally set the generated indices to the mesh 
						mesh.indices = indexbuffer;

						// recompute normals
						DEBUG("   re-computing normals\n");
						computeVertexNormals(model);
					}
				}
			}
			else 
			if (optimize)
			{
				if (optimize)
				{
					for (auto& mesh : model.meshes)
					{
						unsigned int* indices = mesh.indices.data();
						unsigned int indexCount = mesh.indices.size();
						unsigned int vertexCount = mesh.vertices.size();
						PACKED_VERTEX* vertices = mesh.vertices.data(); // xyz == first member 

						DEBUG("   optimizing mesh\n");
						meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
						meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
						meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));
					}
				}
			}
			END_TIMER("Loading '%s' took: ", path)

			DEBUG("   # of vertices  = %d\n", (int)(attrib.vertices.size()) / 3)
			DEBUG("   # of normals   = %d\n", (int)(attrib.normals.size()) / 3)
			DEBUG("   # of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2)
			DEBUG("   # of materials = %d\n", (int)materials.size())
			DEBUG("   # of meshes    = %d\n", (int)model.meshes.size())

			return model;
		}


		void optimizeMesh(MeshInfo& mesh, bool calculateLOD)
		{
			// define lods 
			if (calculateLOD)
			{
				// define lod level 0 
				MeshLODLevelInfo lod0;
				lod0.meshId = mesh.meshId;
				lod0.indexCount = mesh.indices.size();
				lod0.indexOffset = 0;
				lod0.lodLevel = 0;
				mesh.lods.push_back(lod0);

				// no use running this on very small meshes
				if (mesh.indices.size() > 100)
				{
					unsigned int* indices = mesh.indices.data();
					unsigned int indexCount = mesh.indices.size();
					unsigned int vertexCount = mesh.vertices.size();
					PACKED_VERTEX* vertices = mesh.vertices.data(); // xyz == first member 

					// optimize the LOD0 mesh 
					meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
					meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
					meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));
					std::vector<std::vector<unsigned int>> lods = { };

					unsigned int totalIndexCount = indexCount;
					bool noLod = false; 

					for (int i = 1; i < LOD_LEVELS; i++)
					{
						DEBUG("   computing lod %d\n", i);

						float threshold = LOD_THRESHOLDS[i - 1];
						size_t target_index_count = size_t(indexCount * threshold);
						float target_error = LOD_TARGET_ERROR[i - 1];

						std::vector<unsigned int> lodIndices(indexCount);
						float lod_error = 0.f;

						unsigned int lodIndexCount = meshopt_simplify(
							&lodIndices[0],
							indices,
							indexCount,
							&vertices[0].posAndValue.x,
							vertexCount,
							sizeof(PACKED_VERTEX),
							target_index_count,
							target_error,
							/* options= */ 0,
							&lod_error);

						if (lodIndexCount > mesh.indices.size() * 0.95)
						{
							// lod not effective, cancel out 
							noLod = true;
							break;
						}

						lodIndices.resize(lodIndexCount);

						meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndexCount, vertexCount);
						meshopt_optimizeOverdraw(lodIndices.data(), lodIndices.data(), lodIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
						lods.push_back(lodIndices);

						totalIndexCount += lodIndexCount;

						// lod info (filled in below)
						MeshLODLevelInfo lodn;
						lodn.meshId = mesh.meshId;
						lodn.indexCount = lodIndexCount;
						lodn.indexOffset = 0;
						lodn.lodLevel = i;
						mesh.lods.push_back(lodn);

						// next lod level 
						indexCount = lodIndexCount;
						indices = lods[lods.size() - 1].data();
					}
					if (!noLod)
					{
						// rebuild index buffer starting with the smallest lod 
						std::vector<unsigned int> indexbuffer{};
						indexbuffer.reserve(totalIndexCount);

						for (int i = LOD_LEVELS - 1; i >= 1; i--)
						{
							for (int j = 0; j < lods[i - 1].size(); j++)
							{
								indexbuffer.push_back(lods[i - 1][j]);
							}
						}
						for (int j = 0; j < mesh.indices.size(); j++)
						{
							indexbuffer.push_back(mesh.indices[j]);
						}

						// index lot levels in mesh lod info 
						int offset = 0;
						for (int i = LOD_LEVELS - 1; i >= 1; i--)
						{
							MeshLODLevelInfo& lodn = mesh.lods[i];
							lodn.indexOffset = offset;

							unsigned int size = lods[i - 1].size();
							lodn.indexCount = size;
							offset += size;
						}
						mesh.lods[0].indexCount = mesh.indices.size();
						mesh.lods[0].indexOffset = offset;

						// optimize mesh, with lowest lod first ensuring most efficient use on gpu
						meshopt_optimizeVertexFetch(vertices, indexbuffer.data(), totalIndexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));

						// finally set the generated indices to the mesh 
						mesh.indices = indexbuffer;

						// recompute normals
						// computeVertexNormals(model);
					}
				}
			}
			else
			{
				unsigned int* indices = mesh.indices.data();
				unsigned int indexCount = mesh.indices.size();
				unsigned int vertexCount = mesh.vertices.size();
				PACKED_VERTEX* vertices = mesh.vertices.data(); // xyz == first member 

				meshopt_optimizeVertexCache(indices, indices, indexCount, vertexCount);
				meshopt_optimizeOverdraw(indices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX), 1.05f);
				meshopt_optimizeVertexFetch(vertices, indices, indexCount, &vertices[0].posAndValue.x, vertexCount, sizeof(PACKED_VERTEX));
			}
		}
	}
}
