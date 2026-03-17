#pragma once

#include <glm/glm.hpp>

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool load(const char* vert_path, const char* frag_path);
    bool load_compute(const char* comp_path);
    void use() const;

    void set_int(const char* name, int value) const;
    void set_uint(const char* name, unsigned int value) const;
    void set_float(const char* name, float value) const;
    void set_vec3(const char* name, const glm::vec3& v) const;
    void set_mat4(const char* name, const glm::mat4& m) const;

    unsigned int id() const { return program_; }

private:
    unsigned int program_ = 0;
};
