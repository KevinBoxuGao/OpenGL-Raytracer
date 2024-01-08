#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "../include/WindowManager.h"
#include "../include/Enums.h"

using std::cout;
using std::cerr;
using std::endl;

WindowManager::WindowManager(int width, int height, const char* title, bool fullscreen) {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        exit(EXIT_FAILURE);
    }
    video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (fullscreen) {
        window = glfwCreateWindow(video_mode->width, video_mode->height, title, glfwGetPrimaryMonitor(), nullptr);
    }
    else {
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    }
    if (!window) {
        cerr << "Failed to create window" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSizeCallback(window, WindowResizeCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);
    is_fullscreen = fullscreen;

    camera = std::make_shared<Camera>();
    renderer = std::make_unique<Renderer>(window);
    renderer->camera = camera;
}

WindowManager::~WindowManager() {
    cout << "destroying window" << endl;
    glfwDestroyWindow(window);
    glfwTerminate();
}


void WindowManager::ToggleFullscreen() {
    if (glfwGetWindowMonitor(window) == nullptr) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
        is_fullscreen = true;
    }
    else {
        glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 800, 0);
        is_fullscreen = false;
    }
}

bool WindowManager::IsFullscreenMode() {
    return is_fullscreen;
}

void WindowManager::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    // Adjust ImGui viewport size when the window is resized
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    wm->renderer->scene_updated = true;
}

void WindowManager::WindowResizeCallback(GLFWwindow* window, int width, int height) {
    // Implement window resize logic here
}

void WindowManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    // Implement scroll callback logic here
    if (wm->camera && wm->renderer->play_mode) {
        wm->camera->Zoom(yoffset);
    }
}

void WindowManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    // Implement key callback logic here
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            wm->renderer->play_mode = false;
            //glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (key == GLFW_KEY_F) {
            wm->ToggleFullscreen();
        }
        if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D || key == GLFW_KEY_SPACE || key == GLFW_KEY_LEFT_CONTROL) {
            wm->pressed_keys.insert(key);
        }
        if (key == GLFW_KEY_ENTER) {
            wm->renderer->play_mode = !wm->renderer->play_mode;
        }
    }
    // Handle key release
    if (action == GLFW_RELEASE) {
        wm->pressed_keys.erase(key); // Reset the flag when the key is released
    }
}

void WindowManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Implement mouse button callback logic here
}

void WindowManager::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    // Implement mouse move callback logic here
    static double lastX = xpos;
    static double lastY = ypos;

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (wm->camera && wm->renderer->play_mode) {
       wm->camera->Turn(xOffset, yOffset);
    }
}

void WindowManager::RunMainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwMakeContextCurrent(window);
        if (renderer->play_mode && !pressed_keys.empty()) {
            if (pressed_keys.find(GLFW_KEY_W) != pressed_keys.end()) {
                camera->Move(FORWARD);
            }
            if (pressed_keys.find(GLFW_KEY_A) != pressed_keys.end()) {
                camera->Move(LEFT);
            }
            if (pressed_keys.find(GLFW_KEY_S) != pressed_keys.end()) {
                camera->Move(BACKWARD);
            }
            if (pressed_keys.find(GLFW_KEY_D) != pressed_keys.end()) {
                camera->Move(RIGHT);
            }
            if (pressed_keys.find(GLFW_KEY_SPACE) != pressed_keys.end()) {
                camera->Move(UPWARD);
            }
            if (pressed_keys.find(GLFW_KEY_LEFT_CONTROL) != pressed_keys.end()) {
                camera->Move(DOWNWARD);
            }
        }
        renderer->Render(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}