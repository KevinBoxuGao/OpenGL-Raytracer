#include <glm/gtc/matrix_transform.hpp>

#include "../include/Camera.h"


Camera::Camera(
    float width,
    float height,
    float zoom_speed,
    float turn_speed,
    float move_speed,
    float vfov,
    glm::vec3 look_from,
    glm::vec3 look_at
) : window_width{ width }, window_height{ height }, vfov{vfov}, zoom_speed{zoom_speed}, turn_speed{turn_speed}, move_speed{move_speed},
look_from{look_from}, look_at{look_at}
{
    Update();
}

void Camera::Zoom(float amount) {
    vfov -= static_cast<float>(amount) * zoom_speed;
    vfov = glm::clamp(vfov, 1.0f, 120.0f);
}

void Camera::Move(Direction d) {
    glm::vec3 cameraFront = glm::normalize(look_at - look_from);
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, vup));

    switch (d) {
    case FORWARD:
        look_from += move_speed * cameraFront;
        break;
    case BACKWARD:
        look_from -= move_speed * cameraFront;
        break;
    case LEFT:
        look_from -= move_speed * right;
        break;
    case RIGHT:
        look_from += move_speed * right;
        break;
    case UPWARD:
        look_from += move_speed * vup;
        break;
    case DOWNWARD:
        look_from -= move_speed * vup;
        break;
    }
    look_at = look_from + cameraFront;
    Update();
}

void Camera::Turn(float xOffset, float yOffset) {
    xOffset *= turn_speed;
    yOffset *= turn_speed;

    glm::vec3 cameraFront = glm::normalize(look_at - look_from);

    // Calculate the yaw angle
    float yaw = glm::degrees(atan2(cameraFront.z, cameraFront.x));

    // Update the yaw based on mouse movement
    yaw += xOffset;

    // Calculate the pitch angle
    float pitch = glm::degrees(asin(cameraFront.y));

    // Update the pitch based on negated mouse movement (to invert the direction)
    pitch += yOffset;

    // Clamp pitch to limit looking up or down
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    // Convert angles back to radians
    float pitchRad = glm::radians(pitch);
    float yawRad = glm::radians(yaw);

    // Calculate the new camera front vector
    glm::vec3 newFront;
    newFront.x = cos(pitchRad) * cos(yawRad);
    newFront.y = sin(pitchRad);
    newFront.z = cos(pitchRad) * sin(yawRad);

    // Update the look_at position
    look_at = look_from + glm::normalize(newFront);
}


void Camera::Update() {
    camera_center = look_from;

    //viewport dimensions
    float focal_length = glm::length(look_from - look_at);
    float theta = glm::radians(vfov);
    float h = tan(theta / 2);
    float viewport_height = 2 * h * focal_length;
    float viewport_width = viewport_height * (static_cast<float>(window_width) / window_height);

    glm::vec3 w = glm::normalize(look_from - look_at);
    glm::vec3 u = glm::normalize(glm::cross(vup, w));
    glm::vec3 v = glm::cross(w, u);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    glm::vec3 viewport_u = viewport_width * u;
    glm::vec3 viewport_v = viewport_height * v;

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewport_u / (window_width);
    pixel_delta_v = viewport_v / (window_height);

    // Calculate the location of the upper left pixel.
    glm::vec3 viewport_upper_left = camera_center - focal_length*w - (viewport_u / 2.0f) - (viewport_v / 2.0f);
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
}

void Camera::UpdateWindow(float width, float height) {
    window_width = width;
    window_height = height;
    Update();
}

void Camera::Reset() {
    look_at = glm::vec3{ 0,0,0 };
    look_from = glm::vec3{ 0,0,1 };
    vfov = 90;
}