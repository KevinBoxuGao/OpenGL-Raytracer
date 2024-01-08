#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "./include/WindowManager.h"
#include "./include/Renderer.h"

int main() {
    WindowManager windowManager(1280, 800, "OpenGL Program", false);
    windowManager.RunMainLoop();

    return 0;
}