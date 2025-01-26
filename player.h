#pragma once


class Player
{
private:
	VulkanEngine* engine;
	World* world;

	EntityId entityId{ -1 };

	EntityId blockSelectorEntityId{ -1 };
	MaterialId blockSelectorMaterialId{ -1 };
	MeshId blockSelectorMeshId{ -1 };

	FLOAT pitch{ 0 };
	FLOAT yaw{ 0 };
	FLOAT fov{ 45 };
	VEC3 forward{};
	VEC3 up{};
	VEC3 right{}; 

	FLOAT movementSpeed = 10.0f;
	FLOAT mouseSensitivity{ 1.0f };
	FLOAT zoomSpeed{ 100.0f };
	const FLOAT pitchMin = -85.f;
	const FLOAT pitchMax = 85.f;

	void generatePlayerEntity()
	{
		auto texture = engine->initTexture("assets/textures/steve.png");

		auto material = engine->initMaterial("player")
			->setFragmentShader(engine->initFragmentShader("Compiled Shaders/textured.frag.spv").id)
			->setVertexShader(engine->initVertexShader("Compiled Shaders/textured.vert.spv").id)
			->setAlbedoTexture(texture.id)
			->build();

		auto obj = engine->loadObj("assets/steve.obj", engine->getMaterial(material), 1, true, true, true, true);
		entityId = engine->fromModel(obj, false, engine->renderPrototype | ct_player);

		auto aabb = obj.aabb;
		auto pos = VEC4(world->getGroundLevel({ 0.0f, 0.0f }) + VEC3(0.5f, -0.5, 0.5f), 1);
		auto scale = VEC4(2);

		auto mesh = engine->getMesh(obj.meshes[0]);

		engine->setComponentData(entityId, ct_position, pos);
		engine->setComponentData(entityId, ct_scale, scale);
		engine->setComponentData(entityId, ct_rotation, VEC4(0));
		engine->setComponentData(entityId, ct_boundingBox, BBOX::fromAABB(mesh.aabb, pos, scale));
	}
	void generateBlockSelectorEntity()
	{
		if (blockSelectorMaterialId == -1)
		{
			blockSelectorMaterialId = engine->initMaterial("wireframe")
				->setTopology(MaterialTopology::LineList)
				->setLineWidth(3)
				->setVertexShader(engine->initVertexShader(WIREFRAME_VERT_SHADER).id)
				->setFragmentShader(engine->initFragmentShader(WIREFRAME_FRAG_SHADER).id)
				->build();
		}

		if (blockSelectorMeshId == -1)
		{
			MeshInfo mesh;
			mesh.cullDistance = 100;
			mesh.materialId = blockSelectorMaterialId;
			mesh.vertices.resize(12 * 3);
			PACKED_VERTEX* vertices = mesh.vertices.data();

			___GEN_WIREFRAME_CUBE(vertices, VEC4(1, 0, 0, 1), VEC3(0))

				mesh.indices.resize(mesh.vertices.size());
			for (int i = 0; i < mesh.vertices.size(); i++)
			{
				mesh.indices[i] = i;
			}

			mesh.removeDuplicateVertices();
			mesh.quantize();

			blockSelectorMeshId = engine->registerMesh(mesh);
		}

		blockSelectorEntityId = engine->createEntity(engine->renderPrototype);
		engine->setComponentData(blockSelectorEntityId, ct_position, VEC4(0, 0, 0, 0));
		engine->setComponentData(blockSelectorEntityId, ct_scale, VEC4(1));
		engine->setComponentData(blockSelectorEntityId, ct_color, VEC4(1, 0, 0, 1));
		engine->setComponentData(blockSelectorEntityId, ct_mesh_id, blockSelectorMeshId);
		engine->setComponentData(blockSelectorEntityId, ct_material_id, blockSelectorMaterialId);
	}

public: 
	
	inline EntityId getPlayerId() const { return entityId; }

	inline VEC3 getPosition() const {
		return VEC3(*(VEC4*)engine->getComponentData(entityId, ct_position));
	}
	inline void setPosition(VEC3 position) const 
	{
		engine->setComponentData(entityId, ct_position, VEC4(position, 1));
	}

	inline VEC3 getForward() const { return forward; }
	inline VEC3 getUp() const { return up; }
	inline VEC3 getRight() const { return right; }


	void init(VulkanEngine* enginePtr, World* worldPtr)
	{
		engine = enginePtr; 
		world = worldPtr; 

		// create entities 
		generatePlayerEntity(); 
		generateBlockSelectorEntity(); 

		// takeover camera control 
		engine->cameraController.disableControl(); 

		// hide the selector 
		hideBlockSelector(); 
	}					


	void hideBlockSelector()
	{
		if (blockSelectorEntityId >= 0)
		{
			engine->removeComponent(blockSelectorEntityId, ct_render_index); 
		}
	}
	void showBlockSelector() 
	{
		if (blockSelectorEntityId >= 0)
		{
			engine->addComponent(blockSelectorEntityId, ct_render_index);
		}
	}
	void positionBlockSelectorFromScreenXY(VEC2 screenXY)
	{
		int width, height;
		glfwGetWindowSize(engine->window, &width, &height);

		MAT4 p = engine->cameraController.getProjectionMatrix();
		MAT4 v = engine->cameraController.getViewMatrix(); 

		double x_ndc = (2.0 * screenXY.x / width) - 1;
		double y_ndc = (2.0 * screenXY.y / height) - 1;
		MAT4 viewProjectionInverse = INVERSE(p * v);
		VEC4 worldSpacePosition{ x_ndc, y_ndc, 0.0f, 1.0f };
		VEC4 world = viewProjectionInverse * worldSpacePosition;

	    FLOAT x = world.x / world.w;
		FLOAT y = world.y / world.w;

		// cast a ray into the world from cam.z into +z viewspace
		VEC3 camPos = engine->cameraController.getPosition(); 
		VEC3 camForward = engine->cameraController.getForward(); 

		engine->castRay({ VEC3(x, y, camPos.z), camForward }); 

		// todo
		//
		// world->castRayZ(x, y, camPos.z, camForward); 
		//
	}

	void update(FLOAT deltaTime)
	{
		if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyC)) {
			if (engine->cameraController.isControlEnabled())
			{
				engine->cameraController.disableControl(); 
			}
			else
			{
				engine->cameraController.enableControl(); 
			}
		}

		if (!engine->cameraController.isControlEnabled())
		{
			// look
			if (engine->inputManager.isMouseButtonDown(input::MouseButton::right))
			{
				const VEC2 mouseMovement = engine->inputManager.getMouseMovement();
				yaw -= mouseSensitivity * mouseMovement.x;
				pitch += mouseSensitivity * mouseMovement.y;
			}

			// zoom
			FLOAT scroll = engine->inputManager.getMouseScroll();
			if (scroll != 0)
			{
				scroll = scroll * deltaTime * zoomSpeed;
				fov = MAX(2.0f, MIN(90.0f, fov - scroll));
			}
			
			pitch = MIN(MAX(pitch, pitchMin), pitchMax);

			const FLOAT yawRadian = RAD(yaw);
			const FLOAT pitchRadian = RAD(pitch);

			// construct coordinate system
			forward.x = cos(yawRadian) * cos(pitchRadian);
			forward.y = sin(pitchRadian);
			forward.z = sin(yawRadian) * cos(pitchRadian);

			forward = NORM(forward);
			right = NORM(CROSS(forward, VEC3(0.0f, -1.0f, 0.0f))); 
			up = NORM(CROSS(right, forward));

			// update position 
			VEC3 pos = getPosition();
			
			VEC3 velocity = VEC3(0);

			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyW)) { velocity += forward; }
			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyS)) { velocity -= forward; }
			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyA)) { velocity -= right; }
			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyD)) { velocity += right; }
			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keySpacebar)) { /* jump */ }
			if (engine->inputManager.isKeyboardKeyDown(input::KeyboardKey::keyShiftLeft)) { velocity *= 3.0f; }

			VEC3 newpos = pos + velocity * movementSpeed * deltaTime;

			VEC3 grounded = world->getGroundLevel({ newpos.x, newpos.z }) + VEC3(0, 0, 0);

			if (grounded != pos)
			{
				setPosition(grounded);
			}

			auto cam = engine->cameraController.getExtrinsic(); 
			cam.forward = forward;
			cam.position = grounded + VEC3(0, -1.75f, 0) - forward * 3.0f;
			cam.right = right;
			cam.up = up; 			 

			auto view = engine->cameraController.getIntrinsic(); 
			view.fov = fov; 

			engine->cameraController.update(cam, view); 
		}

		// selecting? 
		if (engine->inputManager.isMouseButtonDown(input::MouseButton::left))
		{
			positionBlockSelectorFromScreenXY(engine->inputManager.getMousePosition()); 
			showBlockSelector(); 
		}

		// check for camera changes, trigger update of camera and player systems ifso 
		auto cam = engine->cameraController.getExtrinsic();

		static CameraExtrinsic lastCam = {};

		bool camChanged =
			cam.position != lastCam.position
			||
			cam.forward != lastCam.forward
			||
			cam.up != lastCam.up
			||
			cam.right != lastCam.right;

		lastCam = cam;
		if (camChanged)
		{
			engine->invalidateComponents(ct_camera | ct_player);
		}			  
	}
};