#pragma once

struct WorldChunkGenerationInfo
{
	unsigned long long seed; 
	UINT groundLevel;
	UINT cloudLevel;   

	FastNoise groundNoise; 
	FastNoise cloudNoise; 
	FLOAT worldScale = 1.0f;
    FLOAT heightScale = 40.0f;
    FLOAT cloudChance = 0.4f;
};

class World; // forward 

class WorldChunk
{
public:
	World* world; 

	void allocate()
	{
		if (blocks) std::runtime_error("chunk already allocated");
		blocks = (uint8_t*)malloc(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z);
	}
	void deallocate()
	{
		if (blocks)
		{
			free(blocks);
			blocks = nullptr;
		}
	}

	EntityId entityId{ -1 };
	EntityId borderEntityId{ -1 };

	BYTE* blocks;

	IVEC2 gridXZ; 
	UINT gridIndex;
	VEC3 worldOffset; 

	WorldChunk* leftChunk;	 // -x
	WorldChunk* rightChunk;	 // +x 
	WorldChunk* frontChunk;	 // -z
	WorldChunk* backChunk;	 // +z 

	WorldChunk() 
	{
		world = nullptr;
		entityId = -1;
	}

	WorldChunk(World* worldPtr)
	{
		world = worldPtr;
		entityId = -1; 
	}

	~WorldChunk() { }

	//
	// generate initial blocks 
	//
	void generate(WorldChunkGenerationInfo* genInfo)
	{
		UINT cloudLevel = genInfo->cloudLevel;
		BYTE groundLevel[CHUNK_SIZE_XZ]; 
		bool clouds[CHUNK_SIZE_XZ];

		for (int x = 0; x < CHUNK_SIZE_X; x++)
		{
			for (int z = 0; z < CHUNK_SIZE_Z; z++)
			{
				int index = x * CHUNK_SIZE_Z + z;

				FLOAT n = genInfo->groundNoise.GetCubicFractal(
				 	0.1f * (worldOffset.x + (FLOAT)(x + (gridXZ[0] * CHUNK_SIZE_X))),
					0.1f * (worldOffset.z + (FLOAT)(z + (gridXZ[1] * CHUNK_SIZE_Z))));

clouds[index] = genInfo->cloudNoise.GetCellular
(
	2.0f * worldOffset.x + (FLOAT)(x + (gridXZ[0] * CHUNK_SIZE_X)),
	2.0f * worldOffset.z + (FLOAT)(z + (gridXZ[1] * CHUNK_SIZE_Z))
) > genInfo->cloudChance;

groundLevel[index] = MAX(100.0f, MIN((FLOAT)CHUNK_SIZE_Y, genInfo->groundLevel + ((n + 0.2f) * genInfo->heightScale)));
			}
		}

		for (int y = 0; y < CHUNK_SIZE_Y; y++)
		{
			for (int x = 0; x < CHUNK_SIZE_X; x++)
			{
				for (int z = 0; z < CHUNK_SIZE_Z; z++)
				{
					int idx = y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_X + x;
					int idxPlane = x * CHUNK_SIZE_Z + z;
					int ground = groundLevel[idxPlane];

					if (y < ground)
					{
						blocks[idx] = BT_STONE;
					}
					else
						if (y == ground)
						{
							if (ground < 150) blocks[idx] = BT_GRASS;
							else
								if (ground < 180) blocks[idx] = BT_DIRT;
								else blocks[idx] = BT_SNOW;
						}
						else
						{
							if (y > ground && y < genInfo->cloudLevel)
							{
								blocks[idx] = BT_AIR;
							}
							else
								if (y > genInfo->cloudLevel && y < genInfo->cloudLevel + 3)
								{
									blocks[idx] = clouds[idxPlane] ? BT_CLOUD : BT_AIR;
								}
								else
								{
									blocks[idx] = BT_AIR;
								}
						}
				}
			}
		}

		DEBUG("generated chunk %d at xy: %d, %d\n", entityId, gridXZ.x, gridXZ.y)
	}



	// get/set blocks 
	inline BYTE get(IVEC3 pos, UINT step) const
	{
		return get(pos.x, pos.y, pos.z, step);
	}
	inline BYTE get(int x, int y, int z, UINT step) const
	{
		return get(blocks, x, y, z, step);
	}
	inline BYTE get(uint8_t* data, int x, int y, int z, UINT step) const
	{
		if (x < 0 || x >= CHUNK_SIZE_X) return BT_STONE;
		if (y < 0) return BT_STONE;
		if (y >= CHUNK_SIZE_Y) return BT_AIR;
		if (z < 0 || z >= CHUNK_SIZE_Z) return BT_STONE;

		if (step == 1)
		{
			return data[y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_X + x];
		}
		else
		{
			// select most prevalent blocktype from the section mapped with stepsize 
			std::map< BLOCKTYPE, UINT> v{};
			BLOCKTYPE m = BT_AIR;
			UINT maxn = 0;

			for (int ys = 0; ys < step; ys++)
				for (int zs = 0; zs < step; zs++)
					for (int xs = 0; xs < step; xs++)
					{
						int index = (y + ys) * CHUNK_SIZE_XZ + (z + zs) * CHUNK_SIZE_X + (x + xs); 
						BLOCKTYPE bt = data[index];

						if (bt != BT_AIR)
						{
							// must have air next to it 
							if (   left   (x + xs, y + ys, z + zs, 1) != BT_AIR
								|| right  (x + xs, y + ys, z + zs, 1) != BT_AIR
								|| top    (x + xs, y + ys, z + zs, 1) != BT_AIR
								|| bottom (x + xs, y + ys, z + zs, 1) != BT_AIR
								|| front  (x + xs, y + ys, z + zs, 1) != BT_AIR
								|| back   (x + xs, y + ys, z + zs, 1) != BT_AIR
								)
							{
								if (v.contains(bt))
								{
									UINT n = v[bt] + 1;

									if (n > maxn)
									{
										m = bt;
										maxn = n;
									}

									v[bt] = n;
								}
								else
								{
									v[bt] = 1;
									if (maxn == 0)
									{
										m = bt;
										maxn = 1;
									}
								}
							}
						}
					}
		    
			return m; 
		}			
	}
	inline void set(const IVEC3 pos, const BLOCKTYPE block) const
	{
		blocks[
			pos.y * CHUNK_SIZE_XZ
				+
				pos.z * CHUNK_SIZE_X
				+
				pos.x] = block;
	}
	
	// get a block in a direction from some position crossing over chunk boundary if needed
	BYTE left  (int x, int y, int z, UINT step) const 
	{
		bool crossBorder = x <= (-1 + step); 
		if (leftChunk && crossBorder)
		{
			return get(leftChunk->blocks, x + CHUNK_SIZE_X - step, y, z, step);
		}
		else
		if (crossBorder)
		{
			return BT_STONE;
		}
		else
		{
			return get(x - step, y, z, step);
		}
	}
	BYTE right (int x, int y, int z, UINT step) const 
	{
		if (rightChunk)
		{
			if (x >= CHUNK_SIZE_X - step) return get(rightChunk->blocks, x - CHUNK_SIZE_X + step, y, z, step);
		}
		else
			if (x >= CHUNK_SIZE_X - step) return BT_STONE; 

		return get(x + step, y, z, step);
	}
	BYTE top   (int x, int y, int z, UINT step) const 
	{
		return get(x, y - step, z, step);
	}
	BYTE bottom(int x, int y, int z, UINT step) const 
	{
		return get(x, y + step, z, step);
	}
	BYTE front (int x, int y, int z, UINT step) const 
	{
		if (frontChunk)
		{
			if (z <= (-1 + step)) return get(frontChunk->blocks, x, y, z + CHUNK_SIZE_X - step, step);
		}
		else
			if (z <= (-1 + step)) return BT_STONE;

		return get(x, y, z - step, step);
	}
	BYTE back  (int x, int y, int z, UINT step) const 
	{
		if (backChunk)
		{
			if (z >= CHUNK_SIZE_Z - step) return get(backChunk->blocks, x, y, z - CHUNK_SIZE_Z + step, step);
		}
		else 
			if (z >= CHUNK_SIZE_Z - step) return BT_STONE; 

		return get(x, y, z + step, step);
	}

	// face bitmask for mesh generation 
	BYTE b_top = 0b00000001;
	BYTE b_left = 0b00000010;
	BYTE b_right = 0b00000100;
	BYTE b_bottom = 0b00001000;
	BYTE b_front = 0b00010000;
	BYTE b_back = 0b00100000;


	//
	//  chunk 
	//             
	//     f-------g                
	//    /.      /|                
	//   / .     / |                
	//  e-------h  |                   (a) = origin(0,0,0) of mesh
	//  |  b . .|. c     -y            (a) = 0,0,0 in blocks 
	//  | .     | /       | / +z       (g) = max, max, max in blocks
	//  |.      |/        |/        
	//  a-------d         +---- +x  
	//                              


	void generateMeshSegmentXZ(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* BLOCKS, int x, int y, int z, BYTE face, int i, UINT& c, UINT step)
	{			
		PACKED_VERTEX* vertices = *_vertices; 
		BYTE f = face == b_top ? BOTTOM_FACE : TOP_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = step;
		int joinx = x;
		int joinz = z;
		BLOCKTYPE bt = blocks[i];

		// check to join along the x-axis 
		while (j + x < CHUNK_SIZE_X && bt == blocks[i + j])
		{
			if (faces[i + j] & face)
			{
				faces[i + j] &= ~face;
				joinx = x + j;
				
				j += step;
			}
			else
			{
				break;
			}
		}

		int k = step;
		while (k + z < CHUNK_SIZE_Z && bt == blocks[i + k * CHUNK_SIZE_X])
		{
			bool ok = true;
			for (int jj = step; jj <= joinx - x && ok; jj += step) // scan x axis at z 
			{
				if (bt != blocks[i + jj + k * CHUNK_SIZE_X] || !(faces[i + jj + k * CHUNK_SIZE_X] & face))
				{
					ok = false;
				}
			}
			if (ok)
			{
				joinz = z + k;
				k += step;
			}
			else
			{
				break;
			}
		}

		// draw 1 face over xz
		if (joinx > x || joinz > z)
		{
			// join a square over xz / joinxz, remove the face from the faces array so it wont be redrawn on next z pass
			for (int jj = step; jj <= (joinz - z); jj += step) 
			{
				for (int xi = 0; xi <= joinx - x; xi += step)
				{
					faces[i + CHUNK_SIZE_X * jj + xi] &= ~face;
				}
			}
		}
    
		___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(joinx + (step - 1), 0, joinz + (step - 1)))

		c += 6;
		faces[i] &= ~face;
		*_vertices = vertices; 
	}
	void generateMeshSegmentZY(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* BLOCKS, int x, int y, int z, BYTE face, int i, UINT& c, UINT step)
	{
		PACKED_VERTEX* vertices = *_vertices;
		BYTE f = face == b_left ? LEFT_FACE : RIGHT_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = 1;
		int joinz = z;
		int joiny = y;
		BLOCKTYPE bt = blocks[i];

		// check to join along the z-axis 
		while (
			j + z < CHUNK_SIZE_Z
			&&
			bt == blocks[i + j * CHUNK_SIZE_X]
			&&
			faces[i + j * CHUNK_SIZE_X] & face)
		{
			faces[i + j * CHUNK_SIZE_X] &= ~face;
			joinz = z + j;
			j += 1;
		}

		int k = 1;
		while (k + y < CHUNK_SIZE_Y && bt == blocks[i + k * CHUNK_SIZE_XZ])
		{
			bool ok = true;
			for (int jj = 1; jj <= joinz - z && ok; jj += 1) // scan z axis at y 
			{
				if (bt != blocks[i + jj * CHUNK_SIZE_X + k * CHUNK_SIZE_XZ] || !(faces[i + jj * CHUNK_SIZE_X + k * CHUNK_SIZE_XZ] & face))
				{
					ok = false;
				}
			}
			if (ok)
			{
				joiny = y + k;
				k += 1;
			}
			else
			{
				break;
			}
		}
		
		// draw 1 face over zy
		if (joinz > z || joiny > y)
		{
			// clearout faces avoiding redraw on next y 
			for (int yy = 1; yy <= joiny - y; yy += 1)
			{
				for (int zz = 0; zz <= joinz - z; zz += 1)
				{
					faces[i + zz * CHUNK_SIZE_X + yy * CHUNK_SIZE_XZ] &= ~face;
				}
			}
			___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(0, joiny, joinz))
		}
		else
		// draw 1 face over x
		if (joinz > z)
		{
			___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(0, 0, joinz))
		}
		// draw 1 face over 1 block
		else
		{
			// 1 block, 1 face, 2 triangles
			___GEN_FACE(f, vertices, bt, VEC3(x, -y, z))
		} 

		
		c += 6;
		faces[i] &= ~face;
		*_vertices = vertices;
	}
	void generateMeshSegmentXY(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* BLOCKS, int x, int y, int z, BYTE face, int i, UINT& c, UINT step)
	{
		PACKED_VERTEX* vertices = *_vertices;
		BYTE f = face == b_front ? FRONT_FACE : BACK_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = 1;
		int joinx = x;
		BLOCKTYPE bt = blocks[i];

		// check to join along the x-axis 
		while (j + x < CHUNK_SIZE_X && bt == blocks[i + j] && (faces[i + j] & face))
		{
			faces[i + j] &= ~face;
			joinx = x + j;
			j++;
		}

		int k = 1; 
		int joiny = y; 
		while (k + y < CHUNK_SIZE_Y && bt == blocks[i + k * CHUNK_SIZE_XZ])
		{
			bool ok = true; 
			for (int jj = 1; jj <= joinx - x && ok; jj++) // scan x axis at y 
			{
				if (bt != blocks[i + jj + k * CHUNK_SIZE_XZ] || !(faces[i + jj + k * CHUNK_SIZE_XZ] & face))
				{
					ok = false;
				}
			}
			if (ok)
			{
				joiny = y + k;
				k++;
			}
			else
			{
				break;
			}
		}

		// draw 1 face over xy
		if (joinx > x || joiny > y)
		{
			// clearout faces avoiding redraw on next y 
			for (int yy = 1; yy <= joiny - y; yy++)
			{
				for (int xx = 0; xx <= joinx - x; xx++)
				{
					faces[i + xx + yy * CHUNK_SIZE_XZ] &= ~face; 
				}
			}
			___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(joinx, joiny, 0))
		}
		else
		// draw 1 face over x
		if (joinx > x)
		{	
			___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(joinx, 0, 0))
		}
		// draw 1 face over 1 block
		else
		{
			// 1 block, 1 face, 2 triangles
			___GEN_FACE(f, vertices, bt, VEC3(x, -y, z))
		}

		c += 6;
		faces[i] &= ~face;
		*_vertices = vertices;
	}
	void generateMesh(MeshInfo* mesh, UINT lods = 1)
	{
		const UINT lodLevels = 3;
		
		static PACKED_VERTEX vertexBuffer[1024 * 1024];
		static BYTE faces[CHUNK_SIZE_Y * CHUNK_SIZE_XZ];

		PACKED_VERTEX* vertices = &vertexBuffer[0];
		UINT vertexCount = 0;
		
		UINT step[lodLevels + 1] = { 1, 2, 4, 8 };
		UINT lodOffsets[lodLevels + 1] = { 0, 0, 0, 0 };
		UINT lodIndexCounts[lodLevels + 1] = { 0, 0, 0, 0 };

		std::vector<MeshInfo> submeshes;

		for (int lodLevel = 0; lodLevel <= MIN(lods, lodLevels); lodLevel++)
		{
			UINT currentStep = step[lodLevel];
			lodOffsets[lodLevel] = vertexCount;

			// create a map of the faces to render
			for (int y = 0; y < CHUNK_SIZE_Y; y += currentStep)
			{
				for (int z = 0; z < CHUNK_SIZE_Z; z += currentStep)
				{
					for (int x = 0; x < CHUNK_SIZE_X; x += currentStep)
					{
						UINT i = y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_X + x;
						BYTE face = 0;

						auto bt = (BLOCKTYPE)get(x, y, z, currentStep);
						if (bt != BT_AIR)
						{
							if (left(x, y, z, currentStep) == BT_AIR) face |= b_left;
							if (right(x, y, z, currentStep) == BT_AIR) face |= b_right;
							if (top(x, y, z, currentStep) == BT_AIR) face |= b_top;
							if (bottom(x, y, z, currentStep) == BT_AIR) face |= b_bottom;
							if (front(x, y, z, currentStep) == BT_AIR) face |= b_front;
							if (back(x, y, z, currentStep) == BT_AIR) face |= b_back;
						}

						// if stepsize != 1 the faces array will be filled sparsely 
						faces[i] = face;
					}
				}
			}

			// create triangles from the map of faces joining equal block faces along their axis
			for (int y = 0; y < CHUNK_SIZE_Y; y += currentStep)
			{
				for (int z = 0; z < CHUNK_SIZE_Z; z += currentStep)
				{
					int x; 
					for (x = 0; x < CHUNK_SIZE_X; x += currentStep)
					{
						UINT i = y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_X + x;
						BYTE face = faces[i];

						if (face == 0)
						{
							continue;
						}

						if (face & b_left)
						{
							generateMeshSegmentZY(&vertices, faces, blocks, x, y, z, b_left, i, vertexCount, currentStep);
						}
						if (face & b_right)
						{
							generateMeshSegmentZY(&vertices, faces, blocks, x, y, z, b_right, i, vertexCount, currentStep);
						}
						if (face & b_top)
						{
						    generateMeshSegmentXZ(&vertices, faces, blocks, x, y, z, b_top, i, vertexCount, currentStep);
						}
						if (face & b_bottom)    
						{
							generateMeshSegmentXZ(&vertices, faces, blocks, x, y, z, b_bottom, i, vertexCount, currentStep);
						}
						if (face & b_front)
						{
							generateMeshSegmentXY(&vertices, faces, blocks, x, y, z, b_front, i, vertexCount, currentStep);
						}
						if (face & b_back)
						{
						    generateMeshSegmentXY(&vertices, faces, blocks, x, y, z, b_back, i, vertexCount, currentStep);
						}
					}
				}
			}

			lodIndexCounts[lodLevel] = vertexCount - (lodLevel > 0 ? lodIndexCounts[lodLevel - 1] : 0);
		
			// create a mesh for this lod 
			MeshInfo submesh;
			UINT submeshVertexCount = lodIndexCounts[lodLevel]; 

			submesh.vertices.resize(submeshVertexCount);
			submesh.indices.resize(submeshVertexCount);

			// gen index data
			for (UINT i = 0; i < submeshVertexCount; i++) submesh.indices[i] = i;

			// copy vertex data
			memcpy(submesh.vertices.data(), &vertexBuffer[lodOffsets[lodLevel]], sizeof(PACKED_VERTEX) * submeshVertexCount);

			// dedup vertices 
			submesh.removeDuplicateVertices();

			// store for combining later 
			submeshes.push_back(submesh);
		}

		if (submeshes.size() == 1)
		{	
			// 1 mesh
			mesh->vertices = submeshes[0].vertices;
			mesh->indices = submeshes[0].indices;

			mesh->calculateAABB();
			if (mesh->indices.size() == mesh->vertices.size())
			{
				mesh->removeDuplicateVertices();
			}

			// use meshsimplify to generate lods  
			mesh->lodThresholds		= { 0.8f,  0.6f,   0.1f };
			mesh->lodTargetErrors	= { 0.1f,  0.2f,   0.6f };
			mesh->lodDistances		= { 300,   500,    700 };
			mesh->lodSimplifySloppy = { false,  true,   true };
			mesh->optimizeMesh();
		}
		else
		{
			// combine meshes into 1 
			UINT totalVertexCount = 0; 
			UINT totalIndexCount = 0; 

			for (auto& sub : submeshes)
			{
				totalVertexCount += sub.vertices.size();
				totalIndexCount += sub.indices.size();
			}

			mesh->vertices.resize(totalVertexCount);
			mesh->indices.resize(totalIndexCount);
			mesh->lods.clear(); 

			PACKED_VERTEX* pv = mesh->vertices.data(); 
			UINT* pi = mesh->indices.data(); 
			UINT lodLevel = 0; 
			UINT voffset = 0; 
			UINT ioffset = 0; 

			for (auto& sub : submeshes)
			{
				// setup lod offsets 
				MeshLODLevelInfo lod;
				lod.indexOffset = ioffset; 
				lod.indexCount = sub.indices.size();
				lod.lodLevel = lodLevel;
				lod.meshId = mesh->meshId; 

				mesh->lods.push_back(lod); 

				// copy vertices into mesh
				memcpy(pv, sub.vertices.data(), sub.vertices.size() * sizeof(PACKED_VERTEX));

				// offset and copy indices into mesh 
				for (int i = 0; i < sub.indices.size(); i++)
				{
					pi[i] = sub.indices[i] + voffset; 
				}				
				pv += sub.vertices.size(); 
				pi += sub.indices.size(); 

				DEBUG("  LOD %d: %d vertices\n", lodLevel, sub.vertices.size());

				voffset += sub.vertices.size(); 
				ioffset += sub.indices.size(); 
				lodLevel++; 
			}

			mesh->lodDistances = { 300, 500, 700 };
			mesh->calculateAABB();
		}

		mesh->quantize();
	}

	// todo: we can still save 1/3 of the data by correctly indexing our vertices as we use 6 to draw a face instead of just 4
	static MeshInfo generateBlock(uint8_t blockType, VEC3 offsetPosition = VEC3(0, 0, 0))
	{
		MeshInfo mesh{};

		mesh.vertices.resize(6 * 6);
		PACKED_VERTEX* vertices = mesh.vertices.data();

		___GEN_CUBE(vertices, blockType, offsetPosition);

		mesh.indices.resize(mesh.vertices.size());
		for (int i = 0; i < mesh.vertices.size(); i++)
		{
			mesh.indices[i] = i;
		}

		return mesh;
	}

};

