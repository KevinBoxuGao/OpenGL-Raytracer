#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/Renderer.h"
#include "../include/Shader.h"
#include "../include/Camera.h"

#define MAX_OBJECT_COUNT 128

Renderer::Renderer(GLFWwindow* window, std::shared_ptr<Camera> camera, int light_bounces, int samples_per_pixel, float resolution_factor, bool show_tooltip)
    : camera{ nullptr }, light_bounces{ light_bounces }, samples_per_pixel{ samples_per_pixel }, resolution_factor{ resolution_factor }, show_tooltip{show_tooltip},
    fbo{ 0 }, fbo_texture{ 0 }, scene_updated(true), imgui_initialized(false), play_mode(false) {
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return;
    }
    font_scale = 1;
    SetupScene();
    InitImGui(window);
}

Renderer::~Renderer() {
    ShutdownImGui();
}

//-------------Setting Up Scene Rendering-------------//
void Renderer::SetupScene() {
    SetupTextureAttachment();
    SetupScreenQuad();
    SetupShaders();
    ApplyPreset1();
}

void Renderer::SetupTextureAttachment() {
    float vertices[] = {
        -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };
    GLuint framebuffer_vbo, framebuffer_vao;
    glGenVertexArrays(1, &framebuffer_vao);
    glGenBuffers(1, &framebuffer_vbo);
    glBindVertexArray(framebuffer_vao);
    glBindBuffer(GL_ARRAY_BUFFER, framebuffer_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    vao["ray_tracing"] = framebuffer_vao;
    vbo["ray_tracing"] = framebuffer_vbo;
}

void Renderer::SetupScreenQuad() {
    float quad_vertices[] = {
        -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };

    GLuint quad_vbo, quad_vao;
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    vao["fullscreen_quad"] = quad_vao;
    vbo["fullscreen_quad"] = quad_vbo;
}

void Renderer::SetupShaders() {
    shaders["ray_tracing"] = std::make_unique<Shader>("shaders/raytracing.vs.glsl", "shaders/raytracing.fs.glsl");
    uniform_locations["pixel00"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_pixel00");
    uniform_locations["pixel_delta_u"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_pixelDeltaU");
    uniform_locations["pixel_delta_v"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_pixelDeltaV");
    uniform_locations["camera_center"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_cameraCenter");
    uniform_locations["window_width"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_windowWidth");
    uniform_locations["window_height"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_objects");
    uniform_locations["objects"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_objects");
    uniform_locations["samples_per_pixel"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_samplesPerPixel");
    uniform_locations["light_bounces"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_lightBounces");
    uniform_locations["time"] = glGetUniformLocation(shaders["ray_tracing"]->GetId(), "u_time");

    shaders["fullscreen_quad"] = std::make_unique<Shader>("shaders/fullscreen_quad.vs.glsl", "shaders/fullscreen_quad.fs.glsl");
    uniform_locations["screen_resolution"] = glGetUniformLocation(shaders["fullscreen_quad"]->GetId(), "screenResolution");
}

//-----------Presets----------

void Renderer::ApplyPreset1() {
    scene_objects.clear();
    Material material_ground = { MaterialType::Lambertian, glm::vec3(0.8, 0.8, 0.0), 0, 0 };
    Material material_center = { MaterialType::Lambertian, glm::vec3(0.1, 0.2, 0.5), 0, 0 };
    Material material_left = { MaterialType::Dielectric, glm::vec3(0), 0, 1.5 };
    Material material_right = { MaterialType::Metal, glm::vec3(0.8, 0.6, 0.2), 0.2, 0 };

    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{0, -100.5, -1}, glm::vec3(100), material_ground });
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec3(0.5f), material_center });
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{-1.0f, 0.0f, -1.0f}, glm::vec3(0.5), material_left });
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{-1.0f, 0.0f, -1.0f}, glm::vec3(-0.4), material_left });
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{1.0f, 0.0f, -1.0f}, glm::vec3(0.5f), material_right });

    if (camera) {
        camera->look_at = glm::vec3{ 0,0,0 };
        camera->look_from = glm::vec3{ 0,0,1 };
        camera->vfov = 90;
    }
    scene_updated = true;
}

float randomFloat(float min, float max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distribution(min, max);  

    return distribution(gen);
}

void Renderer::ApplyPreset2() {
    scene_objects.clear();
    Material material_ground = { MaterialType::Lambertian, glm::vec3(0.5, 0.5, 0.5), 0, 0 };
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3{0, -1000, 0}, glm::vec3(1000), material_ground });


    for (int a = -4; a < 4; a++) {
        for (int b = -4; b < 4; b++) {
            auto choose_mat = randomFloat(0, 1);
            glm::vec3 center(a + 0.9 * randomFloat(0, 1), 0.2, b + 0.9 * randomFloat(0, 1));

            if ((center - glm::vec3(4, 0.2, 0)).length() > 0.9) {
                Material sphere_material;
                if (choose_mat < 0.8) {
                    // diffuse
                    glm::vec3 albedo = glm::vec3{ randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1) };
                    sphere_material = { MaterialType::Lambertian, albedo, 0, 0 };
                    scene_objects.push_back({ ObjectType::Sphere, center, glm::vec3(0.2), sphere_material });
                }
                else if (choose_mat < 0.95) {
                    // metal
                    glm::vec3 albedo = glm::vec3(randomFloat(0.5f, 1), randomFloat(0.5f, 1), randomFloat(0.5f, 1));
                    float fuzz = randomFloat(0, 0.5);
                    sphere_material = { MaterialType::Metal, albedo, fuzz, 0 };
                    scene_objects.push_back({ ObjectType::Sphere, center, glm::vec3(0.2), sphere_material });
                }
                else {
                    // glass
                    sphere_material = { MaterialType::Dielectric, glm::vec3(0), 0, 1.5 };
                    scene_objects.push_back({ ObjectType::Sphere, center, glm::vec3(0.2), sphere_material });
                }
            }
        }
    }
    Material material1 = { MaterialType::Dielectric, glm::vec3(0), 0, 1.5 };
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3(0, 1, 0), glm::vec3(1), material1});
    Material material2 = { MaterialType::Lambertian, glm::vec3(0.4, 0.2, 0.1), 0, 0 };
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3(-4, 1, 0), glm::vec3(1), material2 });
    Material material3 = { MaterialType::Metal, glm::vec3(0.7, 0.6, 0.5), 0, 0 };
    scene_objects.push_back({ ObjectType::Sphere, glm::vec3(4, 1, 0), glm::vec3(1), material3 });
 
    camera->look_from = glm::vec3(13, 2, 3);
    camera->look_at = glm::vec3(0, 0, 0);
    camera->vfov = 20;
    scene_updated = true;
}

//---------------------Rendering-----------------
void Renderer::Render(GLFWwindow* window) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int window_width, window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);
    UpdateFontScale(window_width);

    RenderScene(window);
    RenderToolTip(show_tooltip);
    if (play_mode) {
        scene_updated = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else {
        scene_updated = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        RenderSceneSettings();
        RenderCameraSettings();
        RenderPresetsMenu();
        RenderObjectsUI();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

//-------------ImGui-------------//
void Renderer::InitImGui(GLFWwindow* window) {
    if (!imgui_initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        imgui_initialized = true;
    }
}

void Renderer::ShutdownImGui() {
    if (imgui_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
}

void Renderer::RenderSceneSettings() {
    ImGui::Begin("Scene");

    if (ImGui::SliderInt("Max Light Bounces", &light_bounces, 1, 128)) {
        scene_updated = true;
    }

    if (ImGui::SliderInt("Samples per pixel", &samples_per_pixel, 1, 256)) {
        scene_updated = true;
    }

    if (ImGui::SliderFloat("Resolution scale", &resolution_factor, 0.1f, 1.0f)) {
        scene_updated = true;
    }

    ImGui::Checkbox("Show Tooltip", &show_tooltip);

    if (ImGui::Button("Toggle Play Mode")) {
        play_mode = !play_mode;
    }

    ImGui::End();
}

void Renderer::RenderCameraSettings() {
    ImGui::Begin("Camera");
    if (camera) {
        if (ImGui::SliderFloat("move speed", &camera->move_speed, 0.0f, 0.5f)) {
            scene_updated = true; // Set scene_updated if the slider value changes
        }
        if (ImGui::SliderFloat("mouse sensitivity", &camera->turn_speed, 0.0f, 0.1f)) {
            scene_updated = true; // Set scene_updated if the slider value changes
        }
        if (ImGui::SliderFloat("zoom sensitivity", &camera->zoom_speed, 0.0f, 10.0f)) {
            scene_updated = true; // Set scene_updated if the slider value changes
        }
        ImGui::Separator();
        // Edit object position
        if (ImGui::InputFloat3("Position", glm::value_ptr(camera->look_from))) {
            scene_updated = true;
        }
        if (ImGui::InputFloat3("Target", glm::value_ptr(camera->look_at))) {
            scene_updated = true;
        }
        if (ImGui::SliderFloat("FOV", &camera->vfov, 0.0f, 90.0f)) {
            scene_updated = true;
        }
        if (ImGui::Button("Reset Camera")) {
            camera->Reset();
            scene_updated = true;
        }
    }
    else {
        ImGui::Text("Camera not available");
    }
    ImGui::End();
}

void Renderer::RenderPresetsMenu() {
    ImGui::Begin("Presets");
    if (ImGui::Button("Preset 1")) {
        ApplyPreset1();
    }
    if (ImGui::Button("Preset 2")) {
        ApplyPreset2();
    }
    ImGui::End();
}

void Renderer::RenderObjectsUI() {
    ImGui::Begin("Objects");

    for (size_t i = 0; i < scene_objects.size(); ++i) {
        std::string objectName = "Object " + std::to_string(i);

        if (ImGui::TreeNode(objectName.c_str())) {
            bool isObjectModified = false;  // Flag to track if any object is modified

            // Edit object type
            ImGui::Text("Type:");
            const char* objectTypeNames[] = { "None", "Sphere" };
            if (ImGui::Combo(("##Type" + std::to_string(i)).c_str(), (int*)&scene_objects[i].type, objectTypeNames, IM_ARRAYSIZE(objectTypeNames))) {
                isObjectModified = true;
            }

            // Edit object position
            ImGui::Text("Position:");
            if (ImGui::SliderFloat3(("##Position" + std::to_string(i)).c_str(), glm::value_ptr(scene_objects[i].position), -10.0f, 10.0f)) {
                isObjectModified = true;
            }

            switch (scene_objects[i].type) {
            case ObjectType::Sphere:
                // Edit object scale (Radius)
                ImGui::Text("Radius:");
                if (ImGui::SliderFloat(("##ScaleX" + std::to_string(i)).c_str(), &scene_objects[i].scale.x, 0.1f, 100.0f)) {
                    isObjectModified = true;
                }
                break;
            default:
                break;
            }

            // Edit material type
            ImGui::Text("Material Type:");
            const char* materialTypeNames[] = { "None", "Lambertian", "Metal", "Dielectric" };
            if (ImGui::Combo(("##MaterialType" + std::to_string(i)).c_str(), (int*)&scene_objects[i].material.type, materialTypeNames, IM_ARRAYSIZE(materialTypeNames))) {
                isObjectModified = true;
            }

            // Edit material properties based on type
            switch (scene_objects[i].material.type) {
            case MaterialType::Lambertian:
                ImGui::Text("Albedo:");
                if (ImGui::ColorEdit3(("##Albedo" + std::to_string(i)).c_str(), glm::value_ptr(scene_objects[i].material.albedo))) {
                    isObjectModified = true;
                }
                break;

            case MaterialType::Metal:
                ImGui::Text("Albedo:");
                if (ImGui::ColorEdit3(("##Albedo" + std::to_string(i)).c_str(), glm::value_ptr(scene_objects[i].material.albedo))) {
                    isObjectModified = true;
                }
                if (ImGui::SliderFloat(("##Fuzz" + std::to_string(i)).c_str(), &scene_objects[i].material.fuzz, 0.0f, 1.0f)) {
                    isObjectModified = true;
                }
                break;

            case MaterialType::Dielectric:
                ImGui::Text("Refraction Index:");
                if (ImGui::SliderFloat(("##RefractionIndex" + std::to_string(i)).c_str(), &scene_objects[i].material.refraction_index, 1.0f, 3.0f)) {
                    isObjectModified = true;
                }
                break;

            default:
                break;
            }

            // Additional Parameters - Customize this for different object types
            ImGui::Text("Additional Parameters:");
            switch (scene_objects[i].type) {
            case ObjectType::Sphere:
                // You can add specific parameters for the sphere here
                break;
            default:
                break;
            }

            // Button to remove the object
            if (ImGui::Button(("Remove##" + std::to_string(i)).c_str())) {
                scene_objects.erase(scene_objects.begin() + i);
                isObjectModified = true;
                --i; // Adjust index after removal
            }

            ImGui::TreePop();

            // Render again if any object is modified
            if (isObjectModified) {
                scene_updated = true;
            }
        }
    }

    // Add button to add a new object
    if (ImGui::Button("Add Object")) {
        if (scene_objects.size() < MAX_OBJECT_COUNT) {
            scene_objects.push_back(Object());
            scene_updated = true;
        }
    }

    ImGui::End();
}

void Renderer::RenderToolTip(bool is_open) {
    ImGui::Begin("Controls", &is_open);

    ImGui::Text("Keybinds:");
    ImGui::BulletText("ENTER: Toggle play mode");
    ImGui::BulletText("ESC: Exit play mode");
    ImGui::BulletText("F: Toggle Fullscreen");

    ImGui::Text("Movement:");
    ImGui::BulletText("W: Move Forward");
    ImGui::BulletText("A: Move Left");
    ImGui::BulletText("S: Move Backward");
    ImGui::BulletText("D: Move Right");
    ImGui::BulletText("Space: Move Up");
    ImGui::BulletText("LCtrl: Move Down");
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("Mouse: look around");
    ImGui::BulletText("Scrollwheel: zoom");
    

    ImGui::End();
}

void Renderer::UpdateFontScale(int window_width) {
    // Choose a base window width at which the font is at its original size
    const float base_window_width = 1280.0f;

    // Calculate the font scale based on the current window width
    font_scale = window_width / base_window_width;

    // Set the font scale
    ImGui::GetIO().FontGlobalScale = font_scale;
}

//-----------Scene Rendering-------------

void Renderer::RenderScene(GLFWwindow* window) {
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    float window_width = static_cast<float>(framebuffer_width);
    float window_height = static_cast<float>(framebuffer_height);
    
    if (scene_updated) {
        UpdateTexture(window_width, window_height);
    }
    //UpdateTexture(window_width, window_height);
    RenderTexture(window_width, window_height);
}

void Renderer::UpdateTexture(int window_width, int window_height) {
    // Set up FBO with lower resolution
    int lower_resolution_width = window_width * resolution_factor; // Set your desired lower resolution width
    int lower_resolution_height = window_height * resolution_factor; // Set your desired lower resolution height
    UpdateFBO(lower_resolution_width, lower_resolution_height);

    // Render to texture in FBO 
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, lower_resolution_width, lower_resolution_height);
    SendUniforms(lower_resolution_width, lower_resolution_height);
    RenderObjects();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderTexture(int window_width, int window_height) {
    glViewport(0, 0, window_width, window_height);
    SendQuadUniforms(window_width, window_height);
    RenderScreenQuad(fbo_texture);
}

void Renderer::UpdateFBO(int width, int height) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::SendUniforms(float window_width, float window_height) {
    shaders["ray_tracing"]->Use();
    camera->UpdateWindow(window_width, window_height);
    glUniform3fv(uniform_locations["pixel00"], 1, glm::value_ptr(camera->pixel00_loc));
    glUniform3fv(uniform_locations["camera_center"], 1, glm::value_ptr(camera->camera_center));
    glUniform3fv(uniform_locations["pixel_delta_u"], 1, glm::value_ptr(camera->pixel_delta_u));
    glUniform3fv(uniform_locations["pixel_delta_v"], 1, glm::value_ptr(camera->pixel_delta_v));

    glUniform1i(uniform_locations["samples_per_pixel"], samples_per_pixel);
    glUniform1i(uniform_locations["light_bounces"], light_bounces);
    auto current_time_point = std::chrono::high_resolution_clock::now();
    auto duration_since_epoch = current_time_point.time_since_epoch();
    float current_time_in_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count() / 1000.0f;
    glUniform1f(uniform_locations["time"], current_time_in_seconds);
}

void Renderer::RenderObjects() {
    // Objects
    for (size_t i = 0; i < scene_objects.size(); ++i) {
        std::string objectIndex = std::to_string(i);
        // Send type
        glUniform1i(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].type").c_str()), static_cast<int>(scene_objects[i].type));
        // Send position
        glUniform3fv(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].position").c_str()), 1, glm::value_ptr(scene_objects[i].position));
        // Send scale
        glUniform3fv(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].scale").c_str()), 1, glm::value_ptr(scene_objects[i].scale));
        // Send material type
        glUniform1i(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].material.type").c_str()), static_cast<int>(scene_objects[i].material.type));
        // Send material albedo
        glUniform3fv(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].material.albedo").c_str()), 1, glm::value_ptr(scene_objects[i].material.albedo));
        // Send metal fuzziness
        glUniform1f(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].material.fuzz").c_str()), scene_objects[i].material.fuzz);
        // Send refraction index
        glUniform1f(glGetUniformLocation(shaders["ray_tracing"]->GetId(), ("u_objects[" + objectIndex + "].material.refraction_index").c_str()), scene_objects[i].material.refraction_index);
    }
    glBindVertexArray(vao["ray_tracing"]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::SendQuadUniforms(float window_width, float window_height) {
    shaders["fullscreen_quad"]->Use();
    glUniform2f(uniform_locations["screen_resolution"], window_width, window_height);
}

void Renderer::RenderScreenQuad(GLuint texture) {
    glBindVertexArray(vao["fullscreen_quad"]);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}