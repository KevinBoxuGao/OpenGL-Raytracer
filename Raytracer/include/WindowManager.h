#pragma once

#include <GLFW/glfw3.h>

#include <unordered_set>
#include <memory>

#include "./Camera.h"
#include "./Renderer.h"

class WindowManager {
private:
    std::shared_ptr<Camera> camera;
    std::unique_ptr<Renderer> renderer;
    GLFWwindow* window;
    const GLFWvidmode* video_mode;
    bool is_fullscreen;
    std::unordered_set<int> pressed_keys;
public:
    WindowManager(int width, int height, const char* title, bool fullscreen);
    ~WindowManager();
    void ToggleFullscreen();
    bool IsFullscreenMode();

    void RunMainLoop();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowResizeCallback(GLFWwindow* window, int width, int height);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
};
