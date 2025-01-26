#pragma once

struct WorldChunkGenerationInfo
{
	unsigned long long seed; 
	UINT groundLevel;
	UINT cloudLevel;   

	FastNoise groundNoise; 
	FastNoise cloudNoise; 
	FLOAT worldScale = 1.0f;
    FLOAT heightScale = 50.0f;
    FLOAT cloudChance = 0.4f;
};

class World; // forward 

class WorldChunk
{
private:
	enum AllocationType { none, uncompressed, rle };
	enum ChunkState { initial, modified };

	AllocationType blockStorage{ none };
	ChunkState chunkState{ initial };
	SIZE currentAllocationSize{ 0 };

	BYTE* blockMemory = nullptr;

	void allocate()
	{
		if (blockMemory) std::runtime_error("chunk already allocated");

		blockMemory = (BLOCKTYPE*)malloc(blockAllocSize);
		blockStorage = { uncompressed };
		currentAllocationSize = blockAllocSize;

		blocksLod0 = blockMemory;
		blocksLod1 = &blockMemory[lodAllocOffsets[1]];
		blocksLod2 = &blockMemory[lodAllocOffsets[2]];
		blocksLod3 = &blockMemory[lodAllocOffsets[3]];
	}
	void deallocate()
	{
		if (blockMemory)
		{
			free(blockMemory);
			blockStorage = { none };
			blockMemory = nullptr;
			currentAllocationSize = 0;
			blocksLod0 = nullptr;
			blocksLod1 = nullptr;
			blocksLod2 = nullptr;
			blocksLod3 = nullptr;
		}
	}

	void decodeCompressedBlocks(BYTE* destination)
	{
		assert(blockStorage == AllocationType::rle);

		SIZE size = blockAllocSize;
		SIZE i = 0;
		SIZE j = 0;

		while (i < size)
		{
			BYTE n = blockMemory[j++];

			if (n < 251)
			{
				BYTE b = blockMemory[j++];

				for (SIZE k = 0; k < n; k++)
				{
					destination[i++] = b;
				}
			}
			else
				if (n == 251)
				{
					BYTE n2 = blockMemory[j++];
					BYTE b = blockMemory[j++];

					for (SIZE k = 0; k < 250 + n2; k++)
					{
						destination[i++] = b;
					}
				}
				else
					if (n == 254)
					{
						UINT b1 = (UINT)blockMemory[j++];
						UINT b2 = (UINT)blockMemory[j++] << 8;
						UINT b3 = (UINT)blockMemory[j++] << 16;
						UINT b4 = (UINT)blockMemory[j++] << 24;
						BYTE b = blockMemory[j++];

						SIZE l = b1 | b2 | b3 | b4;
						for (SIZE k = 0; k < l; k++)
						{
							destination[i++] = b;
						}
					}
					else
					{
						std::runtime_error("decodeing error");
					}
		}
	}
	std::vector<BYTE> encodeUncompressedBlocks()
	{
		assert(blockStorage == AllocationType::uncompressed);
		assert(currentAllocationSize > 1);

		std::vector<BYTE> data{};
		data.reserve(1024 * 4);

		BYTE current = blockMemory[0];
		SIZE n = 1;

		for (int i = 1; i < currentAllocationSize; i++)
		{
			if (blockMemory[i] == current)
			{
				n++;
			}
			else
			{
				// different char
				if (n < 251)
				{
					data.push_back((BYTE)n);
				}
				else
					if (n < 500)
					{
						data.push_back((BYTE)251);
						data.push_back((BYTE)(n - 250));
					}
					else
					{
						data.push_back((BYTE)254);
						data.push_back((BYTE)(n & 0x000000FF));
						data.push_back((BYTE)((n & 0x0000FF00) >> 8));
						data.push_back((BYTE)((n & 0x00FF0000) >> 16));
						data.push_back((BYTE)((n & 0xFF000000) >> 24));
					}
				data.push_back(current);
				current = blockMemory[i];
				n = 1;
			}
		}
		if (n > 0)
		{
			if (n < 251)
			{
				data.push_back((BYTE)n);
			}
			else
				if (n < 511)
				{
					data.push_back((BYTE)251);
					data.push_back((BYTE)(n - 256));
				}
				else
				{
					data.push_back((BYTE)254);
					data.push_back((BYTE)(n & 0x000000FF));
					data.push_back((BYTE)((n & 0x0000FF00) >> 8));
					data.push_back((BYTE)((n & 0x00FF0000) >> 16));
					data.push_back((BYTE)((n & 0xFF000000) >> 24));
				}
			data.push_back(current);
		}

		return data;
	}

public:
	World* world = nullptr;

	EntityId entityId{ -1 };
	EntityId borderEntityId{ -1 };

	BLOCKTYPE* blocksLod0 = nullptr;
	BLOCKTYPE* blocksLod1 = nullptr;
	BLOCKTYPE* blocksLod2 = nullptr;
	BLOCKTYPE* blocksLod3 = nullptr;

	IVEC2 gridXZ{};
	UINT gridIndex{};
	VEC3 worldOffset{};

	WorldChunk* leftChunk = nullptr;	 // -x
	WorldChunk* rightChunk = nullptr;	 // +x 
	WorldChunk* frontChunk = nullptr;	 // -z
	WorldChunk* backChunk = nullptr;	 // +z 

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

	~WorldChunk() { 
		if (blockMemory != nullptr) deallocate();
	}

	bool isModified() {
		return chunkState == ChunkState::modified; 
	}

	// compress/decompress memory used by this chunk
	void compress()
	{
		if (blockStorage == AllocationType::uncompressed)
		{
			//START_TIMER

			assert(blockMemory != nullptr); 

			SIZE oldSize = currentAllocationSize;
			auto data = encodeUncompressedBlocks(); 
			delete[] blockMemory; 

			blockMemory = new BLOCKTYPE[data.size()];
			currentAllocationSize = data.size(); 

			memcpy(blockMemory, data.data(), data.size());

			blockStorage = { rle };

			//END_TIMER("compressed chunk from %d bytes into %d bytes in ", oldSize, currentAllocationSize)
		}
	}
	bool decompress()
	{
		if (blockStorage == AllocationType::rle)
		{
			assert(blocksLod0 != nullptr); 

			SIZE size = blockAllocSize;
			BYTE* data = new BYTE[size];
			decodeCompressedBlocks(data);

			delete[] blockMemory;
			blockMemory = data;

			blocksLod0 = blockMemory;
			blocksLod1 = &blockMemory[lodAllocOffsets[1]];
			blocksLod2 = &blockMemory[lodAllocOffsets[2]];
			blocksLod3 = &blockMemory[lodAllocOffsets[3]];

			blockStorage = { uncompressed };
			currentAllocationSize = size;
			return true; 
		}

		return false;
	}
	void forget()
	{
		if (blockMemory != nullptr) deallocate(); 
	}

	//
	// generate initial blocks 
	//
	void generate(WorldChunkGenerationInfo* genInfo)
	{
		decompress(); 

		// just return if present 
		if (blockStorage == AllocationType::uncompressed)
		{
			return; 
		}
		
		// alloctype == none, generate it 
		allocate();

		UINT cloudLevel = genInfo->cloudLevel;
		BYTE groundLevel[CHUNK_SIZE_XZ]; 
		bool clouds[CHUNK_SIZE_XZ];

		// generate noise base 
		for (int z = 0; z < CHUNK_SIZE_Z; z++)
			for (int x = 0; x < CHUNK_SIZE_X; x++)
			{
				int index = z * CHUNK_SIZE_X + x;

				FLOAT n = 0.96f * genInfo->groundNoise.GetCubicFractal(
					0.2f * (worldOffset.x + (FLOAT)x),
					0.2f * (worldOffset.z + (FLOAT)z)) 
					+
					0.03f * genInfo->groundNoise.GetCellular(
					2.0f * (worldOffset.x + (FLOAT)x),
					2.0f * (worldOffset.z + (FLOAT)z))					
					+
					0.01f * genInfo->groundNoise.GetCellular(
					4.0f * (worldOffset.x + (FLOAT)x),
					4.0f * (worldOffset.z + (FLOAT)z));

                clouds[index] = genInfo->cloudNoise.GetCellular
                (
                	2.0f * (worldOffset.x + (FLOAT)x),
                	2.0f * (worldOffset.z + (FLOAT)z)) > genInfo->cloudChance;

                groundLevel[index] = MAX(100.0f, MIN((FLOAT)CHUNK_SIZE_Y, genInfo->groundLevel + ((n + 0.1f) * genInfo->heightScale)));
			}

		// generate block data
		for (int y = 0; y < CHUNK_SIZE_Y; y++)
			for (int z = 0; z < CHUNK_SIZE_Z; z++)
				for (int x = 0; x < CHUNK_SIZE_X; x++)
				{
					int idx = y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_X + x;
					int idxPlane = x + CHUNK_SIZE_X * z;
					int ground = groundLevel[idxPlane];

					if (y < ground)
					{
						blocksLod0[idx] = BT_STONE;
					}
					else
						if (y == ground)
						{
							if (ground < 150) blocksLod0[idx] = BT_GRASS;
							else
								if (ground < 180) blocksLod0[idx] = BT_DIRT;
								else blocksLod0[idx] = BT_SNOW;
						}
						else
						{
							if (y > ground && y < genInfo->cloudLevel)
							{
								blocksLod0[idx] = BT_AIR;
							}
							else
								if (y > genInfo->cloudLevel && y < genInfo->cloudLevel + 3)
								{
									blocksLod0[idx] = clouds[idxPlane] ? BT_CLOUD : BT_AIR;
								}
								else
								{
									blocksLod0[idx] = BT_AIR;
								}
						}
				}

		// generate lod block data 
		generateBlockLOD({ 16, 256, 16 }, { 8, 128, 8 }, 2, blocksLod0, blocksLod1);
		generateBlockLOD({  8, 128,  8 }, { 4,  64, 4 }, 2, blocksLod1, blocksLod2);
		generateBlockLOD({  4,  64,  4 }, { 2,  32, 2 }, 2, blocksLod2, blocksLod3);

		chunkState = { initial }; 
		DEBUG("generated chunk %d at xy: %d, %d\n", entityId, gridXZ.x, gridXZ.y)
	}

	void generateBlockLOD(IVEC3 inputSize, IVEC3 outputSize, UINT step, BLOCKTYPE* blockdata, BLOCKTYPE* output)
	{
		UINT inputChunkSizeXZ = inputSize.x * inputSize.z;
		UINT outputChunkSizeXZ = outputSize.x * outputSize.z;

		//int i = 0;
		//int j = 0; 

		for (int y = 0, iy = 0; y < inputSize.y; y += step, iy++)
		{
			for (int z = 0, iz = 0; z < inputSize.z; z += step, iz++)
			{
				for (int x = 0, ix = 0; x < inputSize.x; x += step, ix++)
				{
					// 	i and j should just be incremented,... TODO 
					int i = x + z * inputSize.x + y * inputChunkSizeXZ;
					int j = ix + iz * outputSize.x + iy * outputChunkSizeXZ;

					output[j] = blockdata[i];

					//j++;
					//i += step;
				}
			}
		}
	}

	inline BYTE* lodBlocksFromStep(UINT step)
	{
		switch (step)
		{
		case 1: return blocksLod0;
		case 2: return blocksLod1;
		case 4: return blocksLod2;
		case 8: return blocksLod3;
		}
		assert(false);
	}

	inline SIZE lodLevelFromStep(UINT step)
	{
		switch (step)
		{
		case 1: return 0; 
		case 2: return 1; 
		case 4: return 2; 
		case 8: return 3;
		}
		assert(false);
	}

	inline IVEC3 chunkSizeFromStep(UINT step)
	{
		return lodChunkSizes[lodLevelFromStep(step)];
	}

	// get/set blocks 
	inline BYTE get(IVEC3 pos, UINT step, bool crossesChunk = false)
	{
		return get(pos.x, pos.y, pos.z, step, crossesChunk);
	}
	inline BYTE get(int x, int y, int z, UINT step, bool crossesChunk = false)
	{
		return get(lodBlocksFromStep(step), x, y, z, step, crossesChunk);
	}
	inline BYTE get(uint8_t* data, int x, int y, int z, UINT step, bool crossesChunk = false)
	{
		IVEC3 chunkSize = chunkSizeFromStep(step); 

		if (x < 0 || x >= chunkSize.x) return BT_STONE;
		if (y < 0) return BT_STONE;
		if (y >= chunkSize.y) return BT_AIR;
		if (z < 0 || z >= chunkSize.z) return BT_STONE;

		int i = y * (chunkSize.x * chunkSize.z) + z * chunkSize.x + x; 
		BLOCKTYPE bt = data[i];
		
		// todo:  this should be directional
		if (crossesChunk & (BYTE)(bt != BT_AIR) & (BYTE)(step > 1))
		{
			// check for any piece of air as on lodlevel borders we would otherwise get empty faces 
			bool anyAir = false;
			SIZE cxz = chunkSize.x * chunkSize.z; 
			
			for (int yy = 0; yy < step && yy + y < chunkSize.y && !anyAir; yy++)
			{
				int cxyz = yy * cxz; 
				for (int zz = 0; zz < step && zz + z < chunkSize.z && !anyAir; zz++)
				{
					int ii = i + cxyz + zz * cxz; 
					for (int xx = 0; xx < step && xx + x < chunkSize.x && !anyAir; xx++, ii++)
					{
						if (data[ii] == BT_AIR)
						{
							anyAir = true;
						}
					}
				}
			}
			if (anyAir) bt = BT_AIR;
		}

		return bt; 
	}
	inline void set(const IVEC3 pos, const BLOCKTYPE block) 
	{
		if (blockStorage == AllocationType::rle) decompress(); 
		assert(blockStorage == AllocationType::uncompressed); 

		blocksLod0[
			pos.y * CHUNK_SIZE_XZ
				+
				pos.z * CHUNK_SIZE_X
				+
				pos.x] = block;

		chunkState = { modified };
	}
	
	// get a block in a direction from some position crossing over chunk boundary if needed
	BLOCKTYPE left  (int x, int y, int z, UINT step)   
	{
		BLOCKTYPE bt; 

		bool crossBorder = x <= 0;
		if (leftChunk && crossBorder)
		{
			leftChunk->decompress(); 
			IVEC3 chunkSize = chunkSizeFromStep(step);
			bt = get(leftChunk->lodBlocksFromStep(step), x + chunkSize.x - 1, y, z, step, true);
		}
		else
		if (crossBorder)
		{
			bt = BT_STONE;
		}
		else
		{
			bt = get(x - 1, y, z, step, false);
		}
		return bt; 
	}
	BYTE right (int x, int y, int z, UINT step)  
	{
		IVEC3 chunkSize = chunkSizeFromStep(step);
		if (rightChunk)
		{
			rightChunk->decompress();
			if (x >= chunkSize.x - 1) return get(rightChunk->lodBlocksFromStep(step), x - chunkSize.x + 1, y, z, step, true);
		}
		else
			if (x >= chunkSize.x - 1) return BT_STONE;

		return get(x + 1, y, z, step, false);
	}
	BYTE top   (int x, int y, int z, UINT step)  
	{
		return get(x, y - 1, z, step);
	}
	BYTE bottom(int x, int y, int z, UINT step)  
	{
		return get(x, y + 1, z, step);
	}
	BYTE front (int x, int y, int z, UINT step)  
	{					
		if (frontChunk)
		{		
			if (z <= 0)
			{
				IVEC3 chunkSize = chunkSizeFromStep(step);
				frontChunk->decompress(); 
				return get(frontChunk->lodBlocksFromStep(step), x, y, z + chunkSize.z - 1, step, true);
			}
		}
		else
			if (z <= 0) return BT_STONE;

		return get(x, y, z - 1, step, false);
	}
	BYTE back  (int x, int y, int z, UINT step) 
	{
		IVEC3 chunkSize = chunkSizeFromStep(step);
		if (backChunk)
		{
			if (z >= chunkSize.z - 1)
			{ 
				backChunk->decompress();
				return get(backChunk->lodBlocksFromStep(step), x, y, z - chunkSize.z + 1, step, true);
			}
		}
		else 
			if (z >= chunkSize.z - 1) return BT_STONE;

		return get(x, y, z + 1, step, false);
	}



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


	void generateMeshSegmentXZ(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* blockdata, int x, int y, int z, BYTE face, UINT& c, IVEC3 chunkSize, UINT step)
	{
		PACKED_VERTEX* vertices = *_vertices;
		BYTE f = face == b_top ? BOTTOM_FACE : TOP_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = 1;
		int joinx = x;
		int joinz = z;
		
		BLOCKTYPE bt = *blockdata;

		// check to join along the x-axis 
		while (j + x < chunkSize.x && bt == blockdata[j])
		{
			if (faces[j] & face)
			{
				faces[j] &= ~face;
				joinx = x + j;
				j += 1;
			}
			else
			{
				break;
			}
		}

		int k = 1;
		while (k + z < chunkSize.z && bt == blockdata[k * chunkSize.x])
		{
			bool ok = true;
			for (int jj = 1; jj <= joinx - x && ok; jj += 1) // scan x axis at z 
			{
				int idx = jj + k * chunkSize.x; 
				if (bt != blockdata[idx] || !(faces[idx] & face))
				{
					ok = false;
				}
			}
			if (ok)
			{
				joinz = z + k;
				k += 1;
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
			for (int jj = 1; jj <= (joinz - z); jj += 1)
			{
				for (int xi = 0; xi <= joinx - x; xi += 1)
				{
					faces[chunkSize.x * jj + xi] &= ~face;
				}
			}
		}

		___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(joinx, 0, joinz))

		c += 6;
		*faces &= ~face;
		*_vertices = vertices;
	}
	void generateMeshSegmentZY(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* blockdata, int x, int y, int z, BYTE face, UINT& c, IVEC3 chunkSize, UINT step)
	{
		PACKED_VERTEX* vertices = *_vertices;
		BYTE f = face == b_left ? LEFT_FACE : RIGHT_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = 1;
		int joinz = z;
		int joiny = y;
		BLOCKTYPE bt = *blockdata;

		// check to join along the z-axis 
		while (
			j + z < chunkSize.z
			&&
			bt == blockdata[j * chunkSize.x]
			&&
			faces[j * chunkSize.x] & face)
		{
			faces[j * chunkSize.x] &= ~face;
			joinz = z + j;
			j += 1;
		}

		int k = 1;
		int chunkSizeXZ = chunkSize.x * chunkSize.z; 

		while (k + y < chunkSize.y && bt == blockdata[k * chunkSizeXZ])
		{
			bool ok = true;
			for (int jj = 1; jj <= joinz - z && ok; jj += 1) // scan z axis at y 
			{
				int idx = jj * chunkSize.x + k * chunkSizeXZ; 
				if (bt != blockdata[idx] || !(faces[idx] & face))
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
					faces[zz * chunkSize.x + yy * chunkSizeXZ] &= ~face;
				}
			}
		}

		___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(0, joiny, joinz))

		c += 6;
		*faces &= ~face;
		*_vertices = vertices;
	}
	void generateMeshSegmentXY(PACKED_VERTEX** _vertices, BYTE* faces, BLOCKTYPE* blockdata, int x, int y, int z, BYTE face, UINT& c, IVEC3 chunkSize, UINT step)
	{
		PACKED_VERTEX* vertices = *_vertices;
		BYTE f = face == b_front ? FRONT_FACE : BACK_FACE;

		// reduce triangle count by merging equal blocks along the axis
		int j = 1;
		int joinx = x;
		int k = 1;
		int joiny = y;
		int chunkSizeXZ = chunkSize.x * chunkSize.z;

		BLOCKTYPE bt = *blockdata;

		// check to join along the x-axis 
		while (j + x < chunkSize.x && bt == blockdata[j] && (faces[j] & face))
		{
			faces[j] &= ~face;
			joinx = x + j;
			j += 1;
		}

		while (k + y < chunkSize.y && bt == blockdata[k * chunkSizeXZ])
		{
			bool ok = true;
			for (int jj = 1; jj <= joinx - x && ok; jj += 1) // scan x axis at y 
			{
				int idx = jj + k * chunkSizeXZ;
				if (bt != blockdata[idx] || !(faces[idx] & face))
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

		// draw 1 face over xy
		if (joinx > x || joiny > y)
		{
			// clearout faces avoiding redraw on next y 
			for (int yy = 1; yy <= joiny - y; yy += 1)
			{
				for (int xx = 0; xx <= joinx - x; xx += 1)
				{
					faces[xx + yy * chunkSizeXZ] &= ~face;
				}
			}
		}

		___GEN_FACE_JOIN(f, vertices, bt, VEC3(x, -y, z), VEC3(joinx, joiny, 0))

		c += 6;
		*faces &= ~face;
		*_vertices = vertices;
	}
	void generateLOD(IVEC3 chunkSize, PACKED_VERTEX** vertices, UINT& vertexCount)
	{
		UINT chunkSizeXZ = chunkSize.x * chunkSize.z;
		BYTE* faces = new BYTE[chunkSizeXZ * chunkSize.y];

		UINT step = CHUNK_SIZE_X / chunkSize.x;
		assert(CHUNK_SIZE_Z / chunkSize.z == step);

		BLOCKTYPE* blockdata = lodBlocksFromStep(step);

		// create a map of the faces to render in 
		for (int iy = 0; iy < chunkSize.y; iy++)
			for (int iz = 0; iz < chunkSize.z; iz++)
				for (int ix = 0; ix < chunkSize.x; ix++)
				{
					UINT i = iy * chunkSizeXZ + iz * chunkSize.x + ix;
					BYTE face = 0;

					auto bt = blockdata[i];

					if (bt != BT_AIR)
					{
						// bt is a solid, render if some block bordering it is air
						if (left  (ix, iy, iz, step) == BT_AIR) face |= b_left;
						if (right (ix, iy, iz, step) == BT_AIR) face |= b_right;
						if (top   (ix, iy, iz, step) == BT_AIR) face |= b_top;
						if (bottom(ix, iy, iz, step) == BT_AIR) face |= b_bottom;
						if (front (ix, iy, iz, step) == BT_AIR) face |= b_front;
						if (back  (ix, iy, iz, step) == BT_AIR) face |= b_back;
					}

					faces[i] = face;
				}

		// create triangles from the map of faces joining equal block faces along their axis
		for (int y = 0; y < chunkSize.y; y++)
		{
			int yXZ = y * chunkSizeXZ;
			for (int z = 0; z < chunkSize.z; z++)
			{
				int zX = z * chunkSize.x; 
				int i = yXZ + zX; 
				for (int x = 0; x < chunkSize.x; x++, i++)
				{
					BYTE face = faces[i];
					if ((BYTE)(blockdata[i] == BT_AIR) | (BYTE)(face == 0))
					{
						continue;
					}

					if (face & b_left)
					{
						generateMeshSegmentZY(vertices, &faces[i], &blockdata[i], x, y, z, b_left, vertexCount, chunkSize, step);
					}
					if (face & b_right)
					{
						generateMeshSegmentZY(vertices, &faces[i], &blockdata[i], x, y, z, b_right, vertexCount, chunkSize, step);
					}
					if (face & b_top)
					{
						generateMeshSegmentXZ(vertices, &faces[i], &blockdata[i], x, y, z, b_top, vertexCount, chunkSize, step);
					}
					if (face & b_bottom)
					{
						generateMeshSegmentXZ(vertices, &faces[i], &blockdata[i], x, y, z, b_bottom, vertexCount, chunkSize, step);
					}
					if (face & b_front)
					{
						generateMeshSegmentXY(vertices, &faces[i], &blockdata[i], x, y, z, b_front, vertexCount, chunkSize, step);
					}
					if (face & b_back)
					{
						generateMeshSegmentXY(vertices, &faces[i], &blockdata[i], x, y, z, b_back, vertexCount, chunkSize, step);
					}
				}
			}
		}
		delete[] faces;
	}
	void generateMesh(MeshInfo* mesh, UINT lods = 1)
	{
		decompress(); 
		const UINT lodLevels = 3;
		
		static PACKED_VERTEX vertexBuffer[1024 * 1024];


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
			UINT lodOffset = lodOffsets[lodLevel]; 
			VEC3 chunkSize = lodChunkSizes[lodLevel]; 

			generateLOD(chunkSize, &vertices, vertexCount); 

			if (lodLevel > 0)
			{
				// scale the lods vertices to the correct size 
				VEC3 scale = VEC3(currentStep); 
				for (int i = lodOffset; i < vertexCount; i++)
				{
					vertexBuffer[i].posAndValue = VEC4(vertexBuffer[i].pos() * scale, vertexBuffer[i].posAndValue.w);
				}
			}

			UINT lodVertexCount = vertexCount - lodOffset;
			if (submeshes.size() > 0
				&&
				lodVertexCount > 0.85f * (FLOAT)submeshes[submeshes.size() - 1].vertices.size())
			{
				//DEBUG("no lods could be generated that contained fewer vertices"); 
				//break; 
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
			mesh->lodThresholds		= { 0.8f,  0.5f,  0.1f };
			mesh->lodTargetErrors	= { 0.1f,  0.2f,  0.6f };
			mesh->lodDistances		= { 600,   1000,  1500 };
			mesh->lodSimplifySloppy = { false,  true,  true };
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
			UINT level = 0; 
			UINT voffset = 0; 
			UINT ioffset = 0; 

			for (auto& sub : submeshes)
			{
				// setup lod offsets 
				MeshLODLevelInfo lod;
				lod.indexOffset = ioffset; 
				lod.indexCount = sub.indices.size();
				lod.lodLevel = level;
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

				DEBUG("  LOD %d: %d vertices\n", level, sub.vertices.size());

				voffset += sub.vertices.size(); 
				ioffset += sub.indices.size(); 
				level++;
			}

			mesh->lodDistances = { 500, 800, 1000 };
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

