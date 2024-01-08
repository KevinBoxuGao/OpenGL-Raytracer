#pragma once

#include <string>

class Shader {
private:
    unsigned int id; // Shader program ID
    void CheckCompileErrors(unsigned int shader, const std::string& type);
public:
    // Constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);

    ~Shader();

    // Use/activate the shader
    void Use();

    // Getters
    unsigned int GetId() const;

};
