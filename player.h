#pragma once



class Player
{
private:
	VulkanEngine* engine;
	World* world;

	EntityId entityId{ -1 };

	FLOAT pitch{0};
	FLOAT yaw{0};
	FLOAT fov{ 45 };
	VEC3 forward{};
	VEC3 up{};
	VEC3 right{}; 

	FLOAT movementSpeed = 10.0f;
	FLOAT mouseSensitivity{ 1.0f };
	FLOAT zoomSpeed{ 100.0f };
	const FLOAT pitchMin = -85.f;
	const FLOAT pitchMax = 85.f;

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

		// takeover camera control 
		engine->cameraController.disableControl(); 
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