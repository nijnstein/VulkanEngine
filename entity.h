#pragma once

namespace vkengine
{ 
	typedef int32_t EntityId;
	typedef int64_t ComponentTypeId;
	constexpr ComponentTypeId ALL_COMPONENTS = LLONG_MAX; 

	struct Entity
	{
		EntityId index;
		ComponentTypeId components; 
		bool isStatic;
	};

	struct EntityComponentInfo
	{
		ComponentTypeId id; 
		bool syncToGPU;
		size_t elementSize; 
		bool sparse; 
	};

	struct ComponentSystem
	{
		enum Stage {
			DontCare,
			BeforeFrameUpdate,
			BeforeFrameDraw,
			AfterFrameUpdate,
			AfterFrameDraw,
		};

		ComponentTypeId componentMask;
		Stage stage{DontCare};

		void (*run) (EntityId id) = nullptr;
	};

	class EntityManager
	{
		VulkanDevice* vulkanDevice; 

		std::vector<EntityId> freeEntityIds{}; 

		size_t growStepSize(int current, int diff)
		{
			return (size_t)glm::max(glm::max((int)diff, (int)current / 10), 1000);
		}

		void createBuffers(uint32_t reserveSize = 0)
		{
			destroyBuffers();
			reserveCpuBuffers(reserveSize);

			gpuBuffers.frameBufferCount = MAX_FRAMES_IN_FLIGHT;

			for (auto& c : gpuBuffers.componentBuffers)
			{
				if (c.syncToGPU)
				{
					c.buffers.resize(MAX_FRAMES_IN_FLIGHT);
					c.dirty.resize(MAX_FRAMES_IN_FLIGHT);
					for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) c.dirty[i] = true;
				}
				else
				{
					c.buffers = {};
					c.dirty = {};
				}
			}

			uint32_t c = std::max((uint32_t)1, reserveSize > 0 && reserveSize >= entityCount() ? reserveSize : entityCount());

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				for (auto& cbuffer : gpuBuffers.componentBuffers)
				{
					if (cbuffer.syncToGPU)
					{
						vulkanDevice->createBuffer(
							cbuffer.elementSize * c,
							VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
							VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
							cbuffer.buffers[i]);
					}
				}
			}

			reserveCpuBuffers(c);
		}
		void ensureBufferSizes(uint32_t size)
		{
			if (gpuBuffers.frameBufferCount == 0 || gpuBuffers.frameBufferCount != MAX_FRAMES_IN_FLIGHT)
			{
				destroyBuffers();
				createBuffers(size);
				return;
			}

			uint32_t c = size > 0 && size >= entityCount() ? size : entityCount();

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				for (auto& cbuffer : gpuBuffers.componentBuffers)
				{
					if (cbuffer.syncToGPU)
					{
						uint32_t s = cbuffer.buffers[i].info.size / cbuffer.elementSize;
						if (s < c)
						{
							// dont grow in small steps
							auto diff = c - s;
							auto grow = growStepSize(s, diff);
							auto target = cbuffer.buffers[i].info.size + grow;

							DESTROY_BUFFER(vulkanDevice->allocator, cbuffer.buffers[i])
							DEBUG("growing buffer for component %d for flightframe %d with %d elements to a size of %d bytes", cbuffer.component, i, grow, target * cbuffer.elementSize);

							vulkanDevice->createBuffer(
								cbuffer.elementSize * target,
								VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
								cbuffer.buffers[i]);

							for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) cbuffer.dirty[i] = true;
						}
					}
				}
			}

			reserveCpuBuffers(size);
		}
		void destroyBuffers()
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				for (int i = 0; i < cbuffer.buffers.size(); i++)
				{
					DESTROY_BUFFER(vulkanDevice->allocator, cbuffer.buffers[i])
				}
			}
		}

	protected:

		struct EntityComponentBufferInfo
		{
			ComponentTypeId component;   // flag bit 
			std::vector<Buffer> buffers; // buffers for flight frames 
			std::vector<bool> dirty;     // dirty markers 
			size_t elementSize;          // size in bytes of each element 
			
			void* data;                  // cpu data buffer
			size_t dataCount;            // datasize = (dataCount + reserved) * elementSize 
			size_t free;                 // free number of elements if sparse so list is not grown 1 by 1
			
			bool syncToGPU; 
			bool sparse;                 // uses pointers to data instead of the data itself, does not force unique ids or anything  
		};

		struct EntityBufferPtrs
		{
			std::map<ComponentTypeId, void*> map;
		} cpuBufferPointers;

		struct ComponentSystem
		{
			enum Stage {
				DontCare = 1,
				BeforeFrameUpdate = 2,
				BeforeFrameDraw = 4,
				OnCreate = 8
			};

			std::string name; 
			ComponentTypeId componentMask;
			Stage stage{ DontCare };

			// funcptr to execute on entity data;  entity, component pointers, userdata
			void (*execute) (int, EntityBufferPtrs, void* userDataPtr);
		};

		struct GPUBuffers
		{
			uint32_t frameBufferCount; 
			std::vector<EntityComponentBufferInfo> componentBuffers; 

			Buffer getBuffer(uint32_t frame, ComponentTypeId component)
			{
				for (int i = 0; i < componentBuffers.size(); i++)
					if (componentBuffers[i].component == component)
					{
						if (!componentBuffers[i].syncToGPU)
						{
							std::runtime_error("buffer is not synced to gpu and thus does not exist");
						}
						return componentBuffers[i].buffers[frame];
					}
				return {}; 
			}
			bool anyDirty(uint32_t frame)
			{
				for (int i = 0; i < componentBuffers.size(); i++)
					if (frame < componentBuffers[i].dirty.size() && componentBuffers[i].dirty[frame])
						return true;
				return false;
			}
			bool isDirty(uint32_t frame, ComponentTypeId componentFlags)
			{
				for (int i = 0; i < componentBuffers.size(); i++)
					if (frame < componentBuffers[i].dirty.size() && componentBuffers[i].component & componentFlags)
						return componentBuffers[i].dirty[frame];
				return false; 
			}
			void syncToGPU(uint32_t frame, ComponentTypeId component)
			{
				for (auto& cbuffer : componentBuffers)
					if (cbuffer.syncToGPU && cbuffer.component & component && cbuffer.dirty.size() > 0 && cbuffer.dirty[frame])
					{  
						memcpy(cbuffer.buffers[frame].mappedData, cbuffer.data, cbuffer.elementSize * cbuffer.dataCount);
					    cbuffer.dirty[frame] = false;
						return;
					}		   
			}

		} gpuBuffers;

		std::vector<Entity> entities;
		std::vector<ComponentSystem> systems; 
		
		void initComponent(ComponentTypeId componentId, bool syncToGPU, bool sparse, uint32_t elementSize)
		{
			if (vulkanDevice)
			{
				std::runtime_error("EntityManager.initComponent called after initialization");
			}

			EntityComponentBufferInfo info{};
			info.component = componentId; 
			info.syncToGPU = syncToGPU; 
			info.elementSize = elementSize; 
			info.sparse = sparse; 

			gpuBuffers.componentBuffers.push_back(info); 
		}
		void resetEntityDataInvalidation()
		{
			for (auto& c : gpuBuffers.componentBuffers)
			{
				for (int i = 0; i < c.dirty.size(); i++) c.dirty[i] = false;
			}
		}

		void runSystemStage(ComponentSystem::Stage stage, ComponentTypeId mask, EntityId entity, void* userdata)
		{
			for (auto& system : systems)
			{
				if (system.componentMask & mask == mask)
				{
					system.execute(entity, cpuBufferPointers, userdata); 
				}
			}
		}

	public:

		void initEntityManager(VulkanDevice* device, std::vector<EntityComponentInfo> components, uint32_t reserveSize = 0)
		{
			for (auto& c : components)
			{
				if (c.syncToGPU && c.sparse) {
					std::runtime_error("entitycomponent.sync and sparse are incompatible");
				}

				initComponent(c.id, c.syncToGPU, c.sparse, c.elementSize);
			}

			vulkanDevice = device; 
			
			createBuffers(reserveSize); 
			invalidateComponents(ALL_COMPONENTS); 
		}

		uint32_t entityCount() const { return entities.size() - freeEntityIds.size(); }
	
		void invalidateComponents(ComponentTypeId components)
		{
			for (auto& c : gpuBuffers.componentBuffers)
			{
				if (components & c.component)
				{
					for (int i = 0; i < c.dirty.size(); i++) c.dirty[i] = true;
				}
			}
		}
		bool isEntityDataInvalidated() 
		{
			for (auto& c : gpuBuffers.componentBuffers)
			{
				for(bool b : c.dirty) return true; 
			}
			return false; 
		}

		EntityId createEntity(Entity entity)
		{
			return createEntity(entity, entity.components); 
		}
		EntityId createEntity(ComponentTypeId components)
		{
			return createEntity({.components = components}, components); 
		}
		EntityId createEntity(Entity entity, ComponentTypeId components)
		{
			auto free = freeEntityIds.size(); 
			entity.components = components;

			if (free > 0)
			{
				// take an id from the freelist if there is any 
				entity.index = freeEntityIds[free - 1];
				uint32_t id = (uint32_t)entity.index; 
				freeEntityIds.resize(free - 1); 
				entities[id] = entity;
			}
			else
			{
				// otherwise append a new entity index 
				entity.index = (uint32_t)entities.size();
				entities.push_back(entity);
				reserveCpuBuffers(entities.size()); 
			}
			
			invalidateComponents(ALL_COMPONENTS);
			runSystemStage(ComponentSystem::Stage::OnCreate, entity.components, entity.index, nullptr); 
			onCreateEntity(entity.index);
			return entity.index;
		}
		EntityId fromModel(ModelInfo model, bool isStatic, ComponentTypeId prototype)
		{
			if(model.meshes.size() > 1)
				std::runtime_error("from model not supported for multi mesh models");

			Entity e;
			e.components = prototype | ct_mesh_id | ct_material_id;
			e.isStatic = isStatic;

			EntityId id = createEntity(e);
			setComponentData(id, ct_mesh_id, model.meshes[0]);
			setComponentData(id, ct_material_id, model.materialIds[0]);

			auto aabb = model.aabb; 
			setComponentData(id, ct_boundingBox, BBOX{ VEC4(aabb.min, 1), VEC4(aabb.max, 1) });

			return id; 
		}
		EntityId fromMesh(MeshInfo mesh, bool isStatic, ComponentTypeId prototype)
		{
			Entity e;
			e.components = prototype | ct_mesh_id | ct_material_id;
			e.isStatic = isStatic;

			EntityId id = createEntity(e);
			setComponentData(id, ct_mesh_id, mesh.meshId); 
			setComponentData(id, ct_material_id, mesh.materialId); 

			return id;
		}

		void removeEntity(EntityId id)
		{
			if (id >= 0 && id < entities.size())
			{
				for (auto freeId : freeEntityIds)
				{
					if (freeId == id)
					{
						throw std::runtime_error("removeEntity: entity with given id already in free list");
					}
				}
				entities[id].index = -1;
				freeEntityIds.push_back(id); 
				onRemoveEntity(id); 
			}
		}

		virtual void onCreateEntity(EntityId id) {}
		virtual void onRemoveEntity(EntityId id) {}

		void* getComponentData(ComponentTypeId id)
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.component == id)
				{
					return cbuffer.data;
				}
			}
		}		
		void* getComponentData(EntityId entityId, int id)
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.component == id)
				{
					BYTE* p = (BYTE*)cbuffer.data;

					if (!cbuffer.sparse)
					{
						p += entityId * cbuffer.elementSize;
						return p;
					}
					else
					{
						// need to search for itm, convetion is that sparse elements start with the entity id
						for (int i = 0; i < cbuffer.dataCount; i++)
						{
							if (*(EntityId*)p == entityId)
							{
								return p; 
							}
							p += cbuffer.elementSize; 
						}
						return nullptr; 
					}
				}
			}
			std::runtime_error("entity component type not found"); 
		}
		void* getComponentData(ComponentTypeId id, size_t& dataCount)
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.component == id)
				{
					dataCount = cbuffer.sparse ? cbuffer.dataCount : entityCount(); 
					return cbuffer.data;
				}
			}
		}
	
		template<typename T> void setComponentData(EntityId entity, ComponentTypeId component, T data)
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.component == component)
				{
					if (cbuffer.sparse)
					{
						std::runtime_error("entity manager cannot use setComponentData on a sparse buffer"); 
					}
					
					BYTE* p = (BYTE*)cbuffer.data; 
					p += cbuffer.elementSize * entity; 
					
					memcpy(p, &data, cbuffer.elementSize); 

					invalidateComponents(component); 
				}
			}
		}

		void addComponent(EntityId entityId, ComponentTypeId id)
		{
			entities[entityId].components |= id;
		}
		void addComponentData(ComponentTypeId id, void* data, size_t count, size_t reserve = 50)
		{
			for (auto& cbuffer : gpuBuffers.componentBuffers)
			{
				if (cbuffer.component == id)
				{
					if (!cbuffer.sparse)
					{
						std::runtime_error("can only add components to sparse buffers"); 
					}
					if (cbuffer.dataCount == 0)
					{
						cbuffer.data = malloc((count + reserve) * cbuffer.elementSize); 
						cbuffer.dataCount = count; 
						cbuffer.free = reserve; 
						memcpy(cbuffer.data, data, count * cbuffer.elementSize); 
					}
					else
					if (cbuffer.free >= count)
					{
						uint8_t* dst = (uint8_t*)cbuffer.data; 
						dst += cbuffer.dataCount * cbuffer.elementSize; 
						memcpy(dst, data, count * cbuffer.elementSize); 
						cbuffer.free -= count; 
						cbuffer.dataCount += count; 
					}
					else				 
					{ 
						size_t size = cbuffer.elementSize * (count + cbuffer.dataCount + reserve); 
						uint8_t* old = (uint8_t*)cbuffer.data; 
						cbuffer.data = (uint8_t*)malloc(size);
						memcpy(cbuffer.data, old, cbuffer.dataCount * cbuffer.elementSize); 
						memcpy((uint8_t*)cbuffer.data + cbuffer.dataCount * cbuffer.elementSize, data, count * cbuffer.elementSize);
						free(old);
						cbuffer.dataCount += count; 
						cbuffer.free = reserve;
					}
					invalidateComponents(id); 
				}
			}
		}

		void syncDirty(uint32_t frame) 
		{
			if (!gpuBuffers.anyDirty(frame))
			{
				return; 
			}										   			
			ensureBufferSizes(entityCount()); 
 			gpuBuffers.syncToGPU(frame, ALL_COMPONENTS); 
		};
		void reserveCpuBuffers(uint32_t size)
		{
			if (size == 0) std::runtime_error("entitymanager: trying to reserve 0 size");

			for (auto& info : gpuBuffers.componentBuffers)
			{
				if (size > info.dataCount)
				{
					if (!info.sparse)
					{
						size_t grow = growStepSize(info.dataCount, size - info.dataCount);
						size_t target = info.dataCount + grow;

						if (info.dataCount == 0)
						{
							info.data = malloc(target * info.elementSize);
							info.dataCount = target;
						}
						if (info.dataCount < size)
						{
							void* n = malloc(target * info.elementSize);
							memcpy(n, info.data, info.dataCount * info.elementSize);
							free(info.data);
							info.data = n;
							info.dataCount = target;
						}
					}
				}
			}

			entities.reserve(size);
		}
};
}
