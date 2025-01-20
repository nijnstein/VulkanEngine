#pragma once

namespace vkengine
{
	typedef int32_t WindowId; 

	namespace ui
	{
		static int _windowIdGenerator = 0;

		class Window
		{			
		protected:
			WindowId windowId{ 0 };
			std::string windowName{};

			bool visible{ true };
			bool persistSize{ true };
			bool persistPosition{ true }; 
			bool canClose{ true };

			VEC2 position{ 0 };
			VEC2 extents{ 0 };

			ImGuiWindowFlags flags; 

		public:

			virtual void init(std::string name, VEC2 _position, VEC2 _size, bool _persistSize, bool _persistPosition, ImGuiWindowFlags _flags = ImGuiWindowFlags_None)
			{
				windowId = ui::_windowIdGenerator++;
				persistSize = _persistSize;
				persistPosition = _persistPosition; 
				position = _position;
				extents = _size; 
				windowName = std::move(name);
				flags = _flags;
			}

			const WindowId getWindowId() const { return windowId; }
			const char* getName() const { return windowName.c_str(); }
			const bool isVisible() const { return visible; }

			void show()
			{
				if (!visible)
				{
					visible = true;
					onShow();
				}
			}
			void hide()
			{
				if (visible)
				{
					visible = false;
					onHide();
				}
			}

			virtual void onShow() {}
			virtual void onHide() {}
			virtual void onRender(VulkanEngine* engine, float deltaTime) = 0;
			virtual void onResize(VulkanEngine* engine) = 0;
		
			void render(VulkanEngine* engine, float deltaTime)
			{
				bool open = true;

				ImGui::SetNextWindowPos(
					{ position.x, position.y },
					!persistPosition ? ImGuiCond_Always : ImGuiCond_FirstUseEver
				);
				ImGui::SetNextWindowSize(
					{ extents.x, extents.y },
					!persistSize ? ImGuiCond_Always : ImGuiCond_FirstUseEver
				);
				ImGui::Begin(windowName.c_str(), &open, flags);

				this->onRender(engine, deltaTime);

				ImGui::End();

				if (!open) visible = false;
			}
			void resize(VulkanEngine* engine) 
			{
				onResize(engine); 
			}
		};

		struct UISettings
		{
			#define updateFps 30
			std::array<float, updateFps * 5> frameTimes{};

			bool enableUI = true; 

			float frameTimeMin = 9999.0f;
			float frameTimeMax = 0.0f;
		};

		class UIOverlay
		{
		private:
			ImDrawVert* vtxDst;
			ImDrawIdx* idxDst;

		public:
			VulkanDevice* device;
			VkQueue queue{ VK_NULL_HANDLE };

			VkSampleCountFlagBits rasterizationSamples{ VK_SAMPLE_COUNT_1_BIT };
			uint32_t subpass{ 0 };

			Buffer uiVertexBuffer;
			Buffer uiIndexBuffer;
			int32_t vertexCount{ 0 };
			int32_t indexCount{ 0 };

			std::vector<ShaderInfo> shaders;

			VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
			VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
			VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
			VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
			VkPipeline pipeline{ VK_NULL_HANDLE };

			Image fontImage{ VK_NULL_HANDLE };

			struct PushConstBlock {
				glm::vec2 scale;
				glm::vec2 translate;
			} pushConstBlock;

			bool visible{ true };
			bool updated{ false };
			float scale{ 1.0f };
			float updateTimer{ 0.0f };

			void init(VulkanDevice* vulkanDevice);
			void destroy(); 

			void preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat);
			void prepareResources();

			bool update();
			void draw(const VkCommandBuffer commandBuffer);
			void resize(IVEC2 extent);

			void freeResources();

			bool header(const char* caption);
			bool checkBox(const char* caption, bool* value);
			bool checkBox(const char* caption, int32_t* value);
			bool radioButton(const char* caption, bool value);
			bool inputFloat(const char* caption, float* value, float step, const char* format = "0.3f");
			bool sliderFloat(const char* caption, float* value, float min, float max);
			bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
			bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
			bool button(const char* caption);
			bool colorPicker(const char* caption, float* color);
			void text(const char* formatstr, ...);
		};
	}
}