#include <rtGL/camera.h>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front{ glm::vec3(0.0f, 0.0f, -1.0f) }, MovementSpeed{ D_SPEED }, MouseSensitivity{ D_SENSITIVITY }, FieldOfView{ D_FOV }
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front{ glm::vec3(0.0f, 0.0f, -1.0f) }, MovementSpeed{ D_SPEED }, MouseSensitivity{ D_SENSITIVITY }, FieldOfView{ D_FOV }
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime, bool sprinting)
{
    float velocity = MovementSpeed * deltaTime;
    if (sprinting)
        velocity *= 2.0f;
    
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position.y += velocity;
    if (direction == DOWN)
        Position.y -= velocity;
    
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    FieldOfView -= (float)yoffset;
    if (FieldOfView < 1.0f)
        FieldOfView = 1.0f;
    if (FieldOfView > 45.0f)
        FieldOfView = 45.0f;
}

void Camera::update(glm::vec2& viewportSize)
{
    // restart all matrices
    this->view = this->GetViewMatrix();
    this->projection = glm::perspective(glm::radians(this->FieldOfView), viewportSize.x / viewportSize.y, 0.01f, 100.0f); // set projection
    this->viewProj = projection * view;
    this->viewProjInv = glm::inverse(viewProj);
}