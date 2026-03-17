#version 450 core

in vec2 v_uv;
out vec4 frag_color;

layout(r8ui, binding = 0) readonly uniform uimage2D u_output_tex;

uniform int u_width;
uniform int u_height;
uniform vec3 u_dot_color;

void main() {
    ivec2 coord = ivec2(v_uv * vec2(u_width, u_height));
    coord = clamp(coord, ivec2(0), ivec2(u_width - 1, u_height - 1));
    uint val = imageLoad(u_output_tex, coord).r;
    if (val == 0u) discard;
    frag_color = vec4(u_dot_color, 1.0);
}
