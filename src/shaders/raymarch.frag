#version 450 core

in vec2 v_uv;
out float frag_prob;

uniform mat4 u_inv_view_proj;
uniform vec3 u_camera_pos;
uniform vec3 u_mean;
uniform vec3 u_variance;
uniform float u_step_size;  // minimum step size (dense regions)
uniform float u_march_distance;
uniform int u_kernel;

const float PI = 3.14159265359;
const float MAX_STEP_SCALE = 16.0;
const int MAX_STEPS = 4096;

// ── Density kernels ────────────────────────────────────────────────────────

float gaussian(vec3 p, vec3 mu, vec3 var) {
    vec3 d = p - mu;
    float exponent = -0.5 * (d.x*d.x/var.x + d.y*d.y/var.y + d.z*d.z/var.z);
    float norm = pow(2.0 * PI, 1.5) * sqrt(var.x * var.y * var.z);
    return exp(exponent) / norm;
}

float kernel_gaussian(vec3 p) {
    return gaussian(p, u_mean, u_variance);
}

float kernel_bimodal(vec3 p) {
    vec3 offset = vec3(1.5, 0.0, 0.0) * sqrt(u_variance.x);
    return 0.5 * gaussian(p, u_mean - offset, u_variance)
         + 0.5 * gaussian(p, u_mean + offset, u_variance);
}

float kernel_trimodal(vec3 p) {
    float s = sqrt(u_variance.x) * 1.5;
    // Three peaks in a triangle on the XZ plane
    vec3 m0 = u_mean + vec3(0.0, 0.0, s);
    vec3 m1 = u_mean + vec3(s * 0.866, 0.0, -s * 0.5);
    vec3 m2 = u_mean + vec3(-s * 0.866, 0.0, -s * 0.5);
    return (gaussian(p, m0, u_variance)
          + gaussian(p, m1, u_variance)
          + gaussian(p, m2, u_variance)) / 3.0;
}

float kernel_torus(vec3 p) {
    vec3 d = p - u_mean;
    float R = 2.0 * sqrt(u_variance.x); // major radius
    float r_tube = 0.5 * sqrt(u_variance.y); // tube radius
    // Distance from torus surface (XZ plane ring)
    float ring_dist = length(vec2(d.x, d.z));
    float dist_to_ring = length(vec2(ring_dist - R, d.y));
    float exponent = -0.5 * (dist_to_ring * dist_to_ring) / (r_tube * r_tube);
    return exp(exponent) / (pow(2.0 * PI, 1.5) * r_tube * r_tube * r_tube);
}

// Hydrogen 2p_z orbital: |ψ_{2,1,0}|²
float kernel_orbital_2p(vec3 p) {
    vec3 d = p - u_mean;
    float scale = 1.0 / sqrt(u_variance.x); // controls orbital size
    vec3 r = d * scale;
    float rr = length(r);
    if (rr < 0.001) return 0.0;
    float cos_theta = r.y / rr; // y-axis as z-axis of orbital
    // |ψ_{2,1,0}|² ∝ r² exp(-r) cos²(θ)
    float psi2 = rr * rr * exp(-rr) * cos_theta * cos_theta;
    return psi2 * scale * scale * scale * 0.5; // normalize visually
}

// Hydrogen 3d_z² orbital: |ψ_{3,2,0}|²
float kernel_orbital_3dz2(vec3 p) {
    vec3 d = p - u_mean;
    float scale = 1.0 / sqrt(u_variance.x);
    vec3 r = d * scale;
    float rr = length(r);
    if (rr < 0.001) return 0.0;
    float cos_theta = r.y / rr;
    // Y_2^0 ∝ (3cos²θ - 1)
    float Y20 = 3.0 * cos_theta * cos_theta - 1.0;
    float radial = rr * rr * exp(-rr * 0.667); // 3d radial ∝ r² exp(-r/3*2)
    float psi2 = radial * Y20 * Y20;
    return psi2 * scale * scale * scale * 0.05;
}

// Hydrogen 3d_xy orbital: |ψ_{3,2,±2}|² — four-leaf clover
float kernel_orbital_3dxy(vec3 p) {
    vec3 d = p - u_mean;
    float scale = 1.0 / sqrt(u_variance.x);
    vec3 r = d * scale;
    float rr = length(r);
    if (rr < 0.001) return 0.0;
    float sin_theta = sqrt(r.x*r.x + r.z*r.z) / rr;
    float cos_2phi = 0.0;
    float xz_len = r.x*r.x + r.z*r.z;
    if (xz_len > 0.0001) {
        cos_2phi = (r.x*r.x - r.z*r.z) / xz_len; // cos(2φ)
    }
    // Y_2^2 ∝ sin²θ cos(2φ)
    float Y22 = sin_theta * sin_theta * cos_2phi;
    float radial = rr * rr * exp(-rr * 0.667);
    float psi2 = radial * Y22 * Y22;
    return psi2 * scale * scale * scale * 0.1;
}

float density(vec3 p) {
    switch (u_kernel) {
        case 1: return kernel_bimodal(p);
        case 2: return kernel_trimodal(p);
        case 3: return kernel_torus(p);
        case 4: return kernel_orbital_2p(p);
        case 5: return kernel_orbital_3dz2(p);
        case 6: return kernel_orbital_3dxy(p);
        default: return kernel_gaussian(p);
    }
}

void main() {
    vec2 ndc = v_uv * 2.0 - 1.0;
    vec4 near_clip = u_inv_view_proj * vec4(ndc, -1.0, 1.0);
    vec4 far_clip  = u_inv_view_proj * vec4(ndc,  1.0, 1.0);
    vec3 near_world = near_clip.xyz / near_clip.w;
    vec3 far_world  = far_clip.xyz / far_clip.w;

    vec3 ray_dir = normalize(far_world - near_world);
    vec3 ray_pos = u_camera_pos;

    float prob = 0.0;
    float t = 0.0;
    float prev_density = 0.0;

    for (int i = 0; i < MAX_STEPS && t < u_march_distance; ++i) {
        vec3 p = ray_pos + ray_dir * t;
        float d = density(p);

        // Adaptive step: use density to determine step size
        // High density → base step size, zero density → large steps
        // Use max of current and previous density for smooth transitions
        float d_ref = max(d, prev_density);
        // Map density to step scale: density 0 → MAX_STEP_SCALE, high density → 1
        float scale = mix(MAX_STEP_SCALE, 1.0, clamp(d_ref * 50.0, 0.0, 1.0));
        float step = u_step_size * scale;

        prob += d * step;
        prev_density = d;
        t += step;
    }

    frag_prob = prob;
}
