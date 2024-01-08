#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/Enums.h"

class Camera {
private:
    float window_width;
    float window_height;
    glm::vec3 vup = glm::vec3{ 0,1,0 };
public:
    float zoom_speed;
    float turn_speed;
    float move_speed;
    glm::vec3 look_from;
    glm::vec3 look_at;
    float vfov;

    glm::vec3 camera_center;
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;
    glm::vec3 pixel00_loc;

    Camera(
        float width = 0,
        float height = 0,
        float zoom_speed = 5.0f,
        float turn_speed = 0.05f,
        float move_speed = 0.25f,
        float vfov = 90.0f,
        glm::vec3 look_from = glm::vec3{ 0,0,1 },
        glm::vec3 look_at = glm::vec3{ 0,0,0 }
    );
    void Update();
    void UpdateWindow(float width, float height);

    void Zoom(float amount);
    void Move(Direction d);
    void Turn(float xOffset, float yOffset);

    void Reset();
};
