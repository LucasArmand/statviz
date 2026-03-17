#include "app/Shader.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <string>

Shader::~Shader() {
    if (program_) glDeleteProgram(program_);
}

static std::string read_file(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        spdlog::error("Failed to open shader file: {}", path);
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static unsigned int compile_shader(const char* source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        spdlog::error("Shader compilation failed: {}", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static unsigned int link_program(unsigned int* shaders, int count) {
    unsigned int program = glCreateProgram();
    for (int i = 0; i < count; ++i)
        glAttachShader(program, shaders[i]);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        spdlog::error("Shader link failed: {}", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool Shader::load(const char* vert_path, const char* frag_path) {
    std::string vs_src = read_file(vert_path);
    std::string fs_src = read_file(frag_path);
    if (vs_src.empty() || fs_src.empty()) return false;

    unsigned int vs = compile_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int fs = compile_shader(fs_src.c_str(), GL_FRAGMENT_SHADER);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    unsigned int shaders[] = {vs, fs};
    program_ = link_program(shaders, 2);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program_ != 0;
}

bool Shader::load_compute(const char* comp_path) {
    std::string cs_src = read_file(comp_path);
    if (cs_src.empty()) return false;

    unsigned int cs = compile_shader(cs_src.c_str(), GL_COMPUTE_SHADER);
    if (!cs) return false;

    program_ = link_program(&cs, 1);

    glDeleteShader(cs);
    return program_ != 0;
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::set_int(const char* name, int value) const {
    int loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform1i(loc, value);
}

void Shader::set_uint(const char* name, unsigned int value) const {
    int loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform1ui(loc, value);
}

void Shader::set_float(const char* name, float value) const {
    int loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform1f(loc, value);
}

void Shader::set_vec3(const char* name, const glm::vec3& v) const {
    int loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(v));
}

void Shader::set_mat4(const char* name, const glm::mat4& m) const {
    int loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}
