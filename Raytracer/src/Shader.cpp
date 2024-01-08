#pragma once

#include <GL/glew.h>

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "../include/Shader.h"

Shader::Shader(const char* vertex_file_path, const char* fragment_file_path) {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    printf(vertex_file_path);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else {
        std::cerr << "Could not open vertex shader file: " << vertex_file_path << std::endl;
        id = 0;
        return;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }
    else {
        std::cerr << "Could not open fragment shader file: " << vertex_file_path << std::endl;
        id = 0;
        return;
    }

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);
    CheckCompileErrors(id, "VERTEX");

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);
    CheckCompileErrors(id, "FRAGMENT");

    // Shader Program
    printf("Linking program\n");
    id = glCreateProgram();
    glAttachShader(id, VertexShaderID);
    glAttachShader(id, FragmentShaderID);
    glLinkProgram(id);
    CheckCompileErrors(id, "PROGRAM");

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
}

// Destructor to clean up shader program
Shader::~Shader() {
    glDeleteProgram(id);
}

// Utility function for checking shader compilation and linking errors
void Shader::CheckCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[512];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Error: " << type << " shader compilation failed\n" << infoLog << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Error: Shader program linking failed\n" << infoLog << std::endl;
        }
    }
}

// Use/activate the shader
void Shader::Use() {
    glUseProgram(id);
}

//Getters
unsigned int Shader::GetId() const {
    return id;
}
