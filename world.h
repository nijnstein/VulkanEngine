#pragma once

#include "defines.h"
#include "block.h"
#include "worldChunk.h" 
#include "consolewindow.h"

class World
{
private:
	struct MeshRequestInfo
	{
		World* world; 
		WorldChunk* chunk; 
	};

	const UINT MAX_WORLD_CHUNK_CX = 1024 * 256;
	const UINT MAX_WORLD_CHUNK_CZ = 1024 * 256;

	IVEC2 initialWorldSize{ 0, 0 };
	IVEC2 gridMin{ 0, 0 };
	IVEC2 gridMax{ 0, 0 }; 
	VEC4 worldOffset{ 0 };

	std::map<UINT, WorldChunk> chunks;

	MaterialId materialId{ -1 };
	MeshId bboxMeshId{ -1 };
	MaterialId bboxMaterialId{ -1 }; 

	bool enableBorders = false; 


public:
	WorldChunkGenerationInfo generationInfo
	{
		.seed = 1337,
		.groundLevel = 140,
		.cloudLevel = 240,
		.groundNoise = FastNoise(),
		.cloudNoise = FastNoise(),
		.heightScale = 90.0f,
		.cloudChance = 0.4f
	};

	World()
	{
		generationInfo.groundNoise.SetSeed(generationInfo.seed);
		generationInfo.groundNoise.SetFrequency(0.025f);
		generationInfo.groundNoise.SetNoiseType(FastNoise::NoiseType::Cubic);
		generationInfo.groundNoise.SetFractalType(FastNoise::FractalType::FBM);
		
		generationInfo.cloudNoise.SetSeed(generationInfo.seed);
		generationInfo.cloudNoise.SetFrequency(0.005f);
		generationInfo.cloudNoise.SetNoiseType(FastNoise::NoiseType::Cellular);
		generationInfo.cloudNoise.SetFractalType(FastNoise::FractalType::RigidMulti);	
		generationInfo.cloudNoise.SetCellularReturnType(FastNoise::CellularReturnType::CellValue);
		generationInfo.cloudNoise.SetCellularDistanceFunction(FastNoise::CellularDistanceFunction::Natural);
	}
	~World(){}

	void setMaterialId(MaterialId id) { materialId = id; } 
	MaterialId getMaterialId() const { return materialId; }

	BBOX getWorldDimensions() { 
		return {
			VEC4(gridMin[0] * CHUNK_SIZE_X, 0, gridMin[1] * CHUNK_SIZE_Y, 1),
			VEC4(gridMax[0] * CHUNK_SIZE_X, CHUNK_SIZE_Y,gridMax[1] * CHUNK_SIZE_Y, 1)
		};
	}
	IVEC2 getChunkXZFromWorldXYZ(VEC3 worldXYZ)
	{
		VEC3 p = VEC4(worldXYZ, 1) - worldOffset;
		IVEC2 xz { p.x / CHUNK_SIZE_X, p.z / CHUNK_SIZE_Z };
		return xz; 
	}	
	VEC3 getGroundLevel(VEC2 xzWorld) 
	{
		IVEC2 chunkXY = { (int)xzWorld.x / CHUNK_SIZE_X, (int)xzWorld.y / CHUNK_SIZE_Z };
		IVEC2 xy = { (int)xzWorld.x % CHUNK_SIZE_X, (int)xzWorld.y % CHUNK_SIZE_Z };

		// start at cloud level to get highest ground level at pos
		BLOCKTYPE* pillar = &getChunk(chunkXY)->blocks[(CHUNK_SIZE_Y - 1) * CHUNK_SIZE_XZ + xy.y * CHUNK_SIZE_X + xy.x];

		for (int i = CHUNK_SIZE_Y - 1; i >= 0; i--)
		{
			switch (*pillar)
			{
			case BT_AIR:
			case BT_CLOUD:
				pillar -= CHUNK_SIZE_XZ;
				continue;

			default: 
				return VEC3(xzWorld.x, (CHUNK_SIZE_Y - i), xzWorld.y);
			}
		}

		return VEC3(0); 
	}

	WorldChunk* getChunk(UINT index)
	{
		return &chunks[index];
	}
	WorldChunk* getChunk(IVEC2 gridXZ)
	{
		int x = gridXZ[0];
		int z = gridXZ[1];
		int	index = x * MAX_WORLD_CHUNK_CZ + z;

    	return &chunks[index];
	}
	WorldChunk* createChunk(IVEC2 gridXZ)
	{
		int x = gridXZ[0];
		int z = gridXZ[1];
		int	id = x * MAX_WORLD_CHUNK_CZ + z;

		gridMin = { MIN(gridMin[0], x), MIN(gridMin[1], z) };
		gridMax = { MAX(gridMax[0], x), MAX(gridMax[1], z) };

		WorldChunk chunk = WorldChunk(this);
		chunk.allocate();

		chunk.gridXZ = gridXZ;
		chunk.gridIndex = id;
		chunk.worldOffset = VEC3(x * CHUNK_SIZE_X + worldOffset.x, worldOffset.y, z * CHUNK_SIZE_Z + worldOffset.x);
		
		chunks[id] = chunk;

		return &chunks[id]; 
	}

	// 
	// initialize first chunks, allocates space for blocks and sets worldsize and origin 
	//
	void initChunks(VulkanEngine* engine, IVEC2 fromXZ, IVEC2 untilXZ)
	{
		initialWorldSize = { untilXZ[0] - fromXZ[0] + 1, untilXZ[1] - fromXZ[1] + 1 };
		worldOffset = VEC4(-CHUNK_SIZE_X * (initialWorldSize[0] / 2), CHUNK_SIZE_Y, -CHUNK_SIZE_Z * (initialWorldSize[1] / 2), 1);

		// allocate a max of nx * nz  
		for (int x = fromXZ[0]; x <= untilXZ[0]; x++)
		{
			for (int z = fromXZ[1]; z <= untilXZ[1]; z++)
			{
				createChunk(IVEC2(x, z));
			}
		}

		setMaterialId(engine->initMaterial
		(
			"phong-material",
			-1, -1, -1, -1,
			engine->initVertexShader(PHONG_VERTEX_SHADER).id,
			engine->initFragmentShader(PHONG_FRAGMENT_SHADER).id
		).materialId);

		generateEntities(engine);
	}
 
    // generate entities for chunks that have none 
	void generateEntities(VulkanEngine* engine)
	{
		for (int x = gridMin[0]; x <= gridMax[0]; x++)
		{
			for (int z = gridMin[1]; z <= gridMax[1]; z++)
			{
				auto chunk = getChunk({ x, z });
				chunk->entityId = generateChunkEntity(engine, { x, z });
			}
		}
	}

	void enableChunkBorders(VulkanEngine* engine)
	{
		if (!enableBorders)
		{
			for (auto& [UINT, chunk] : chunks)
			{
				if (chunk.entityId >= 0 && chunk.borderEntityId == -1)
				{
					chunk.borderEntityId = generateChunkBorderEntity(engine, chunk.gridXZ);
				}
			}
			enableBorders = true;
		}
	}
	void disableChunkBorders(VulkanEngine* engine)
	{
		if (enableBorders)
		{
			for (auto& [UINT, chunk] : chunks)
			{
				if (chunk.borderEntityId >= 0)
				{
					engine->removeEntity(chunk.borderEntityId); 
					chunk.borderEntityId = -1;
				}
			}

			enableBorders = false; 
		}
	}
	inline bool chunkBordersEnabled() const {
		return enableBorders;
	}
  	VEC4 getChunkBorderColorFromAxis(int x, int z)
	{
		if (x < 0 && z < 0)	    return VEC4(1, 0, 0, 1);
		if (x >= 0 && z >= 0)   return VEC4(0, 1, 0, 1);
		if (x < 0 && z >= 0)	return VEC4(0, 0, 1, 1);
		/*if (x >= 0 && z < 0)*/return VEC4(1, 0, 1, 1);
	}

	EntityId generateChunkBorderEntity(VulkanEngine* engine, IVEC2 xz) 
	{
		int x = xz[0];
		int z = xz[1];
		WorldChunk* wc = getChunk(xz);

		VEC4 borderColor = getChunkBorderColorFromAxis(wc->worldOffset.x, wc->worldOffset.z); 

		BBOX box = BBOX
		{
			VEC4(wc->worldOffset, 0) - VEC4(0, CHUNK_SIZE_Y, 0, 0),
			VEC4(wc->worldOffset, 0) + VEC4(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z, 0)
		};
		box.align();

		if (bboxMaterialId < 0)
		{
			bboxMaterialId = engine->initMaterial("bbox-wireframe")
				->setTopology(MaterialTopology::LineList)
				->setLineWidth(2)
				->setVertexShader(engine->initVertexShader(WIREFRAME_VERT_SHADER).id)
				->setFragmentShader(engine->initFragmentShader(WIREFRAME_FRAG_SHADER).id)
				->build(); 														 		
		}

		if (bboxMeshId < 0)
		{
			MeshInfo mesh;
			mesh.cullDistance = 100; 
			mesh.materialId = bboxMaterialId;
			mesh.vertices.resize(12 * 3);
			PACKED_VERTEX* vertices = mesh.vertices.data();

			___GEN_WIREFRAME_CUBE(vertices, borderColor, VEC3(0))

			mesh.indices.resize(mesh.vertices.size());
			for (int i = 0; i < mesh.vertices.size(); i++)
			{
				mesh.indices[i] = i;
			}

			mesh.removeDuplicateVertices();
			mesh.scaleVertices({ CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z });
			mesh.translateVertices({ 0, CHUNK_SIZE_Y, 0 });
			mesh.setVertexColors(VEC4(1));
			mesh.quantize(); 

			bboxMeshId = engine->registerMesh(mesh); 
		}

		EntityId entityId = engine->createEntity(engine->renderPrototype | ct_chunk_id); 
		engine->setComponentData(entityId, ct_boundingBox, box); 
		engine->setComponentData(entityId, ct_position, VEC4(box.min.x, 0, box.min.z, 0));
		engine->setComponentData(entityId, ct_scale, VEC4(1));
		engine->setComponentData(entityId, ct_color, borderColor);
		engine->setComponentData(entityId, ct_mesh_id, bboxMeshId); 
		engine->setComponentData(entityId, ct_material_id, bboxMaterialId); 
		engine->setComponentData(entityId, ct_chunk_id, wc->entityId); 

		return entityId; 
	}

	EntityId generateChunkEntity(VulkanEngine* engine, IVEC2 xz)
	{
		int x = xz[0]; 
		int z = xz[1]; 
		WorldChunk* wc = getChunk(xz);

		BBOX box = BBOX
		{
			VEC4(wc->worldOffset, 0) - VEC4(0, CHUNK_SIZE_Y, 0, 0),
			VEC4(wc->worldOffset, 0) + VEC4(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z, 0)
		};
		box.align(); 

		auto prototype = ct_position | ct_rotation | ct_scale | ct_chunk_id | ct_boundingBox | ct_chunk | ct_mesh_id; 

		Chunk chunk{};
		chunk.chunkId = wc->entityId = engine->createEntity(prototype);
		chunk.min = box.min;
		chunk.max = box.max;
		chunk.gridXZ = xz;

		// gen block data
		wc->generate(&generationInfo);

		// connect grid pieces for later mesh generation 
		if (x > 0)			wc->leftChunk  = getChunk({ x - 1, z });
		if (x < gridMax[0]) wc->rightChunk = getChunk({ x + 1, z });
		if (z > 0)			wc->frontChunk = getChunk({ x, z - 1 });
		if (z < gridMax[1]) wc->backChunk  = getChunk({ x, z + 1 });
		
		MeshInfo mesh;
		mesh.materialId = getMaterialId(); 
		mesh.userdataPtr = wc; 
		mesh.requestMesh = requestMesh; 
		mesh.aabb = AABB::fromBoxMinMax(VEC3(box.min), VEC3(box.max));
		mesh.meshId = engine->registerMesh(mesh); 
	
		engine->registerMesh(mesh); 
		engine->setComponentData(wc->entityId, ct_position,		wc->worldOffset);
		engine->setComponentData(wc->entityId, ct_rotation,		QUAT(0, 0, 0, 0));
		engine->setComponentData(wc->entityId, ct_scale,		VEC4(1));
		engine->setComponentData(wc->entityId, ct_boundingBox,	box);
		engine->setComponentData(wc->entityId, ct_chunk_id,		chunk.chunkId);
		engine->setComponentData(wc->entityId, ct_mesh_id,      mesh.meshId); 
		engine->setComponentData(wc->entityId, ct_material_id,	mesh.materialId); 
		engine->addComponentData(ct_chunk, &chunk, 1);

		if (enableBorders)
		{
			wc->borderEntityId = generateChunkBorderEntity(engine, { x, z });
		}

		return wc->entityId; 
	}

	static void requestMesh(void* enginePtr, MeshInfo* mesh, void* userdataPtr)
	{
		VulkanEngine* engine = (VulkanEngine*)enginePtr; 
		WorldChunk* chunk = (WorldChunk*)userdataPtr;

		chunk->generateMesh(mesh);
		engine->setComponentData(chunk->entityId, ct_boundingBox, BBOX
			{
				VEC4(mesh->aabb.min + chunk->worldOffset, 1),
				VEC4(mesh->aabb.max + chunk->worldOffset, 1)
			});
	}

	MeshId generateChunkMesh(VulkanEngine* engine, IVEC2 xz)
	{
		auto chunk = getChunk(xz);

		MeshId meshId = *(MeshId*)engine->getComponentData(chunk->entityId, ct_mesh_id); 
	
		requestMesh(engine, &engine->meshes[meshId], chunk);

		return meshId; 
	}

};
