#pragma once

namespace vkengine
{
    struct CameraExtrinsic
    {
        VEC3 position = VEC3(0.f, -5.f, 5.f);
        VEC3 forward = VEC3(0.f, 0.f, 1.f);
        VEC3 right = VEC3(1.f, 0.f, 0.f);
        VEC3 up = VEC3(0.f, -1.f, 0.f);
        VEC3 lookatTarget = VEC3(0);
    };

    struct CameraIntrinsic
    {
        float fov = 80.f;
        float aspectRatio = 1.f;
        float near = 0.1f;
        float far = 200.f;
        float viewPortWidth = 1;
        float viewPortHeight = 1;
    };

    enum class CameraType
    {
        Fps = 0,
        ArcBall = 1
    };

    class CameraController {
    public:
        void init(CameraExtrinsic extrinsic, CameraIntrinsic intrinsic, CameraType type);
        bool update(input::InputManager inputManager, float deltaTime);

        CameraExtrinsic getExtrinsic();
        CameraIntrinsic getIntrinsic();

        //controls
        float m_movementSpeed = 10.0f;
        float m_mouseSensitivity = 1.0f;
        float m_controllerSensitivity = 1.f;
        float m_sprintSpeedFactor = 1.5f;
        float m_scrollSpeed = 10000.0f; 

        CameraType getCameraType() const { return m_cameraType; }
        MAT4 getViewMatrix() const { return view; }
        MAT4 getProjectionMatrix() const { return projection; }
        VEC3 getViewDir() const { return -glm::transpose(view)[2]; }
        VEC3 getVelocity() const { return velocity; }
        MAT4 getRotationMatrix() const { return rotation; }
        VEC3 getPosition() const { return m_extrinsic.position; }
        VEC3 getWorldPosOfMouse(VEC2 mousePosition);
        MAT4 getViewProjectionMatrix() const { return viewProjection; }

        void setCameraType(CameraType type)
        {
            m_cameraType = type; 
            updateMatrices(); 
        }

        void setViewport(IVEC2 extent)
        {
            m_intrinsic.viewPortWidth = extent.x;
            m_intrinsic.viewPortHeight = extent.y;
            m_intrinsic.aspectRatio = extent.x / (float)extent.y;
            updateMatrices(); 
        }

    private:
        const float pitchMin = -85.f;
        const float pitchMax = 85.f;

        float m_pitch = 0.f;
        float m_yaw = -90.f;

        CameraType m_cameraType = CameraType::ArcBall; 
        CameraExtrinsic m_extrinsic;
        CameraIntrinsic m_intrinsic;

        VEC2 m_lastMousePosition{};
        VEC3 velocity{};

        MAT4 view{};
        MAT4 projection{};
        MAT4 rotation{};
        MAT4 viewProjection{};

        void updateMatrices(bool includeRotation = true)
        {
            switch (m_cameraType)
            {
            case CameraType::ArcBall:
            {
                view = glm::lookAt(m_extrinsic.position, m_extrinsic.lookatTarget, m_extrinsic.up);
                break;
            }

            case CameraType::Fps:
            {
                view = glm::lookAt(m_extrinsic.position, m_extrinsic.position + m_extrinsic.forward, m_extrinsic.up);
                break;
            }
            }

            glm::quat pitchRotation = glm::angleAxis(m_pitch, VEC3{ 1.f, 0.f, 0.f });
            glm::quat yawRotation = glm::angleAxis(m_yaw, VEC3{ 0.f, -1.f, 0.f });
        
            rotation = glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
            projection = glm::perspective(glm::radians(m_intrinsic.fov), m_intrinsic.aspectRatio, m_intrinsic.near, m_intrinsic.far);
            projection[1][1] *= -1;

            viewProjection = projection * view;
        }


        bool updateFpsCamera(input::InputManager inputManager, float deltaTime);
        bool updateArcBallCamera(input::InputManager inputManager, float deltaTime);
    };
}

