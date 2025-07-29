#pragma once

#include <glm/glm.hpp>

enum MovementDirections
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class Camera
{
   public:
    glm::vec3 position;
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw   = -90.0f;
    float pitch = 0.0f;

    float movementSpeed    = 50.0f;
    float mouseSensitivity = 0.1f;
    float zoom             = 90.0f;

    Camera(glm::vec3 position = glm::vec3(0.0f, 100.0f, 0.0f),
           glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix(float aspectRatio = 4.0f / 3.0f, float nearPlane = 0.1f,
                                  float farPlane = 500.0f) const;
    void      processKeyboard(MovementDirections direction, float deltaTime);
    void      processMouseMovement(float xOffset, float yOffset);

   private:
    void updateCameraVectors();
};