#include "defines.h"

using namespace vkengine::input;

namespace vkengine
{
	CameraExtrinsic CameraController::getExtrinsic()
	{
		return m_extrinsic;
	}

	CameraIntrinsic CameraController::getIntrinsic()
	{
		return m_intrinsic;
	}

	void CameraController::init(CameraExtrinsic extrinsic, CameraIntrinsic intrinsic, CameraType type)
	{
		m_extrinsic = extrinsic;
		m_intrinsic = intrinsic;
		m_cameraType = type; 
	}

	bool CameraController::updateFpsCamera(input::InputManager inputManager, float deltaTime)
	{
		if (m_controlEnabled)
		{
			// look
			if (inputManager.isMouseButtonDown(MouseButton::right))
			{
				const VEC2 mouseMovement = inputManager.getMouseMovement();
				m_yaw -= m_mouseSensitivity * mouseMovement.x;
				m_pitch += m_mouseSensitivity * mouseMovement.y;
			}

			// zoom
			FLOAT scroll = inputManager.getMouseScroll();
			if (scroll != 0)
			{
				scroll = scroll * deltaTime * m_scrollSpeed;
				m_intrinsic.fov = MAX(2.0f, MIN(90.0f, m_intrinsic.fov - scroll));
			}
			m_pitch = MIN(MAX(m_pitch, pitchMin), pitchMax);

			const FLOAT yawRadian = RAD(m_yaw);
			const FLOAT pitchRadian = RAD(m_pitch);

			//construct coordinate system
			m_extrinsic.forward.x = cos(yawRadian) * cos(pitchRadian);
			m_extrinsic.forward.y = sin(pitchRadian);
			m_extrinsic.forward.z = sin(yawRadian) * cos(pitchRadian);
			m_extrinsic.forward = NORM(m_extrinsic.forward);
		}

		m_extrinsic.right = NORM(CROSS(m_extrinsic.forward, VEC3(0.0f, -1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		m_extrinsic.up = NORM(CROSS(m_extrinsic.right, m_extrinsic.forward));

		// handle keyboard (if enabled)
		velocity = VEC3(0);
		if (m_controlEnabled)
		{
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyW)) { velocity += m_extrinsic.forward; }
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyS)) { velocity -= m_extrinsic.forward; }
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyA)) { velocity -= m_extrinsic.right; }
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyD)) { velocity += m_extrinsic.right; }
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keySpacebar)) { /* jump */ }
			if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyShiftLeft)) { velocity *= 3.0f; }
		}

		m_extrinsic.position = this->m_extrinsic.position + velocity * m_movementSpeed * deltaTime;
		m_extrinsic.lookatTarget = m_extrinsic.position + m_extrinsic.forward;

		updateMatrices(); 
		return true; 
	}

	bool CameraController::updateArcBallCamera(input::InputManager inputManager, float deltaTime)
	{																   
		// handle mouse updates 
		glm::vec2 mousePosition = inputManager.getMousePosition();
		if (!inputManager.isMouseButtonDown(MouseButton::left) || (m_lastMousePosition.x == 0 && m_lastMousePosition.y == 0))
		{
			m_lastMousePosition = mousePosition; 
		}

		// handle keyboard 
		velocity = VEC3(0);
		VEC3 lookAtTargetVelocity = VEC3(0); 

		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyW)) { velocity.z = 1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyS)) { velocity.z = -1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyA)) { velocity.x = -1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyD)) { velocity.x = 1; }
		
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keySpacebar)) { velocity.y = 1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyShiftLeft)) { velocity.y = -1; }
		
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyLeft)) { lookAtTargetVelocity.x = -1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyRight)) { lookAtTargetVelocity.x = 1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyUp)) { lookAtTargetVelocity.z = -1; }
		if (inputManager.isKeyboardKeyDown(input::KeyboardKey::keyDown)) { lookAtTargetVelocity.z = 1; }
		
		velocity *= m_movementSpeed * deltaTime;
		lookAtTargetVelocity *= m_movementSpeed * deltaTime;

		// get all vectors 
		glm::vec3 eye = m_extrinsic.position;
		glm::vec3 forward = m_extrinsic.forward;
		glm::vec3 up = m_extrinsic.up;
		glm::vec3 right = m_extrinsic.right;
		glm::vec3 viewDir = -glm::transpose(getViewMatrix())[2];
						
		float width = m_intrinsic.viewPortWidth;
		float height = m_intrinsic.viewPortHeight;
		constexpr float pi = glm::pi<float>();

		// Get the homogenous position of the camera and pivot point
		glm::vec4 position(eye.x, eye.y, eye.z, 1);
		glm::vec4 pivot(m_extrinsic.lookatTarget.x, m_extrinsic.lookatTarget.y, m_extrinsic.lookatTarget.z, 1);

		// step 1 : Calculate the amount of rotation given the mouse movement.
		float deltaAngleX = (2 * pi / width);  // a movement from left to right = 2*PI = 360 deg
		float deltaAngleY = (pi / height);     // a movement from top to bottom = PI = 180 deg
		float xAngle = (m_lastMousePosition.x - mousePosition.x) * deltaAngleX + velocity.x * deltaAngleX * pi * 2;
		float yAngle = (m_lastMousePosition.y - mousePosition.y) * deltaAngleY + velocity.y * deltaAngleY * pi;

		// Extra step to handle the problem when the camera direction is the same as the up vector
		float cosAngle = glm::dot(viewDir, up);
		if (cosAngle * glm::sign(deltaAngleY) > 0.99f) deltaAngleY = 0;

		// step 2: Rotate the camera around the pivot point on the first axis.
		glm::mat4x4 rotationMatrixX(1.0f);
		rotationMatrixX = glm::rotate(rotationMatrixX, xAngle, up);
		position = (rotationMatrixX * (position - pivot)) + pivot;

		// step 3: Rotate the camera around the pivot point on the second axis.
		glm::mat4x4 rotationMatrixY(1.0f);
		rotationMatrixY = glm::rotate(rotationMatrixY, yAngle, right);
		glm::vec3 finalPosition = (rotationMatrixY * (position - pivot)) + pivot;

		// Update the camera view (we keep the same lookat and the same up vector)
		m_extrinsic.position = std::move(finalPosition);
		m_extrinsic.forward = forward;
		m_extrinsic.up = up;
		m_extrinsic.right = glm::normalize(glm::cross(m_extrinsic.up, m_extrinsic.forward));

		// update position of camera based on keyboard movement speed
		m_extrinsic.position = m_extrinsic.position + m_extrinsic.forward * velocity;
		m_extrinsic.lookatTarget = m_extrinsic.lookatTarget + lookAtTargetVelocity;

		updateMatrices();

		// Update the mouse position for the next rotation
		m_lastMousePosition.x = mousePosition.x;
		m_lastMousePosition.y = mousePosition.y;

		return true; 
	}


	bool CameraController::update(input::InputManager inputManager, float deltaTime)
	{
		switch (m_cameraType)
		{
			case CameraType::Fps:
				return updateFpsCamera(inputManager, deltaTime);

			case CameraType::ArcBall:
				return updateArcBallCamera(inputManager, deltaTime); 
		}
	}

	void CameraController::update(CameraExtrinsic cam, CameraIntrinsic view)
	{
		this->m_extrinsic = cam;
		this->m_intrinsic = view; 
		updateMatrices();
	}

	VEC3 CameraController::getWorldPosOfMouse(VEC2 mousePosition)
	{
		float x_ndc = (2.0 * mousePosition.x / m_intrinsic.viewPortWidth) - 1;
		float y_ndc = (2.0 * mousePosition.y / m_intrinsic.viewPortHeight) - 1;

		MAT4 viewProjectionInverse = glm::inverse(getProjectionMatrix() * getViewMatrix());
		VEC4 worldSpacePosition(x_ndc, y_ndc, 0.0f, 1.0f);
		VEC4 world = viewProjectionInverse * worldSpacePosition;

		return VEC3(
			world.x * abs(m_extrinsic.position.z),
			world.y * abs(m_extrinsic.position.z),
			0); 
	}
}
