#pragma once

#include <GLFW/glfw3.h>

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../include/Shader.h"
#include "../include/Camera.h"

// Enumerations
enum class ObjectType {
    NullObject,
    Sphere
};

enum class MaterialType {
    NullMaterial,
    Lambertian,
    Metal,
    Dielectric
};

// Structs
struct Material {
    MaterialType type;
    glm::vec3 albedo;
    float fuzz;
    float refraction_index;
};

struct Object {
    ObjectType type;
    glm::vec3 position;
    glm::vec3 scale;
    Material material;
};

class Renderer {
private:
    // Private Members
    float font_scale;
    bool imgui_initialized;

    // Shader-related members
    std::unordered_map<std::string, GLuint> uniform_locations;
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;

    // OpenGL buffers
    std::unordered_map<std::string, GLuint> vbo;
    std::unordered_map<std::string, GLuint> vao;
    std::unordered_map<std::string, GLuint> ibo;
    GLuint fbo;
    GLuint fbo_texture;

    // Scene settings
    int light_bounces;
    int samples_per_pixel;
    float resolution_factor;
    bool show_tooltip;

    // Objects in the scene
    std::vector<Object> scene_objects;

    // Private Methods
    //scene setup
    void SetupScene();
    void SetupTextureAttachment();
    void SetupScreenQuad();
    void SetupShaders();
    //presets
    void ApplyPreset1();
    void ApplyPreset2();

    //imgUI
    void InitImGui(GLFWwindow* window);
    void ShutdownImGui();
    void RenderUI();
    void RenderSceneSettings();
    void RenderCameraSettings();
    void RenderPresetsMenu();
    void RenderObjectsUI();
    void RenderToolTip(bool is_open);
    void UpdateFontScale(int window_width);
    //scene rendering
    void RenderScene(GLFWwindow* window);
    void UpdateTexture(int window_width, int window_height);
    void RenderTexture(int window_width, int window_height);

    void UpdateFBO(int width, int height);
    void SendUniforms(float window_width, float window_height);
    void RenderObjects();
    void SendQuadUniforms(float window_width, float window_height);
    void RenderScreenQuad(GLuint texture);

public:
    // Public Members
    bool scene_updated;
    bool play_mode;
    std::shared_ptr<Camera> camera;

    // Public Methods
    Renderer(GLFWwindow* window, 
        std::shared_ptr<Camera> camera = nullptr, 
        int light_bounces = 20,
        int samples_per_pixel = 64, 
        float resolution_factor = 0.5f, 
        bool show_tooltip = true
    );
    ~Renderer();
    void Render(GLFWwindow* window);
};
