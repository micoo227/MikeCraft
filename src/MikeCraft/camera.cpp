#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up) : position(position), up(up)
{
    worldUp = up;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
}

void Camera::processKeyboard(MovementDirections direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD)
    {
        position += forward * velocity;
    }
    if (direction == BACKWARD)
    {
        position -= forward * velocity;
    }
    if (direction == LEFT)
    {
        position -= right * velocity;
    }
    if (direction == RIGHT)
    {
        position += right * velocity;
    }
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;
    yaw += xOffset;
    pitch += yOffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 newForward;
    newForward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newForward.y = sin(glm::radians(pitch));
    newForward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward      = glm::normalize(newForward);
    right        = glm::normalize(glm::cross(forward, worldUp));
    up           = glm::normalize(glm::cross(right, forward));
}
