#pragma once

namespace vkengine
{
	class MaterialBuilder;
	typedef int32_t MaterialId; 

	enum class MaterialTopology
	{
		TriangleList, 
		LineList
	};

	struct Material
	{
		std::string name; 
		MaterialId materialId{ -1 };
	
		int32_t albedoTextureId = -1;
		int32_t normalTextureId = -1;
		int32_t metallicTextureId = -1;
		int32_t roughnessTextureId = -1;

		int32_t vertexShaderId = -1; 
		int32_t fragmentShaderId = -1; 

		MaterialTopology topology{ MaterialTopology::TriangleList };
		float lineWidth = 1; 

		bool hasAlbedoTexture() 
		{
			return albedoTextureId >= 0;
		}

		bool hasNormalTexture()
		{
			return normalTextureId >= 0;
		}

		bool hasMetallicTexture() 
		{
			return metallicTextureId >= 0;
		}

		bool hasRoughnessTexture()
		{
			return roughnessTextureId >= 0;
		}

		void getBindings(std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			if (albedoTextureId >= 0)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
				.binding = (uint32_t)bindings.size(),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.pImmutableSamplers = nullptr
					});
			}

			if (normalTextureId >= 0)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding = (uint32_t)bindings.size(),
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
					});
			}

			if (metallicTextureId >= 0)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding = (uint32_t)bindings.size(),
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
					});
			}

			if (roughnessTextureId >= 0)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding = (uint32_t)bindings.size(),
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
					});
			}
		}

		void getDescriptors(ITextureManager* tm, std::vector<VkWriteDescriptorSet>& descriptorSets)
		{	
			// Albedo Texture
			if (albedoTextureId >= 0)
			{
				descriptorSets.push_back(
					tm->getTexture(albedoTextureId, true, true)
					.getDescriptor((uint32_t)descriptorSets.size()));
			}

			// Normal texture
			if (normalTextureId >= 0)
			{
				descriptorSets.push_back(
					tm->getTexture(normalTextureId, true, true)
					.getDescriptor((uint32_t)descriptorSets.size()));
			}

			if (metallicTextureId >= 0)
			{
				descriptorSets.push_back(
					tm->getTexture(metallicTextureId, true, true)
					.getDescriptor((uint32_t)descriptorSets.size()));
			}

			if (roughnessTextureId >= 0)
			{
				descriptorSets.push_back(
					tm->getTexture(roughnessTextureId, true, true)
					.getDescriptor((uint32_t)descriptorSets.size()));
			}
		}
	};

	class IMaterialManager
	{
	public:
		virtual MaterialId registerMaterial(Material& material) = 0;
		virtual MaterialBuilder* initMaterial(std::string name) = 0;
		virtual Material getMaterial(MaterialId material) = 0;
	};

	class MaterialBuilder
	{
	private:
		IMaterialManager* manager;
		Material current;

	public:
		MaterialBuilder(IMaterialManager* materialManager)
		{
			manager = materialManager;
		}

		MaterialBuilder* create(std::string name)
		{
			current = {};
			current.name = name;
			return this;
		}
		MaterialBuilder* setAlbedoTexture(TextureId id)
		{
			current.albedoTextureId = id;
			return this;
		}
		MaterialBuilder* setNormalTexture(TextureId id)
		{
			current.normalTextureId = id;
			return this;
		}
		MaterialBuilder* setMetallicTexture(TextureId id)
		{
			current.metallicTextureId = id;
			return this;
		}
		MaterialBuilder* setRoughnessTexture(TextureId id)
		{
			current.roughnessTextureId = id;
			return this;
		}
		MaterialBuilder* setVertexShader(ShaderId id)
		{
			current.vertexShaderId = id;
			return this;
		}
		MaterialBuilder* setFragmentShader(ShaderId id)
		{
			current.fragmentShaderId = id;
			return this;
		}
		MaterialBuilder* setTopology(MaterialTopology topology)
		{
			current.topology = topology;
			return this;
		}
		MaterialBuilder* setLineWidth(float width)
		{
			current.lineWidth = width; 
			return this; 
		}
		MaterialBuilder* setId(MaterialId id)
		{
			current.materialId = id;
			return this;
		}
		MaterialId build(bool dispose = true)
		{
			MaterialId id{ -1 };
			if (manager)
			{
				id = current.materialId = manager->registerMaterial(current);
			}
			if (dispose)
			{
				delete this;  // :)
			}
			return id;
		}
	};


}