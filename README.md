# statviz

A real-time 3D probability distribution visualizer using stochastic ray marching on the GPU. Rather than rendering smooth volumes, statviz lights up individual pixels proportional to the underlying probability density, producing a shimmering pointillist effect where density naturally emerges from randomness.

![OpenGL 4.5](https://img.shields.io/badge/OpenGL-4.5-blue)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![License](https://img.shields.io/badge/license-MIT-green)


https://github.com/user-attachments/assets/e20cb576-8f1b-44b4-9679-1b9dee62c18b


## How It Works

The renderer executes a three-pass GPU pipeline every frame:

1. **Ray March (fragment shader):** A fullscreen quad reconstructs world-space rays from NDC via the inverse view-projection matrix. Each fragment marches through the scene, accumulating `density(p) * step_size` into an R32F probability texture. Step size is adaptive -- small in dense regions, up to 16x larger in empty space -- based on the local density value.

2. **Normalize + Sample (compute shader, 2 dispatches):** The first dispatch sums all per-pixel probabilities into a shared SSBO using fixed-point atomic addition. After a barrier, the second dispatch computes each pixel's normalized probability, multiplies by the target sample count N, and runs a Bernoulli trial against a PCG-hashed random number. The result is a binary R8UI texture.

3. **Display (fragment shader):** A fullscreen quad reads the binary texture and outputs the configured dot color for lit pixels, discarding unlit ones. The grid and ImGui overlay render on top.

Because the random seed incorporates the frame counter, each frame produces a different sample pattern. At high N the distribution appears solid; at low N individual stochastic samples shimmer across the density field.

## Distributions

Seven density kernels are available, selectable at runtime:

| Kernel | Description |
|--------|-------------|
| Gaussian | Single 3D Gaussian with configurable mean and diagonal covariance |
| Bimodal Gaussian | Two offset Gaussian peaks |
| Trimodal Gaussian | Three peaks arranged in a triangle |
| Torus | Ring-shaped distribution on the XZ plane |
| Orbital 2p | Hydrogen 2p_z dumbbell shape |
| Orbital 3d_z2 | Hydrogen 3d_z² with equatorial ring and axial lobes |
| Orbital 3d_xy | Four-leaf clover pattern |

New kernels can be added by writing a `float kernel_foo(vec3 p)` function in `raymarch.frag` and extending the switch statement.

## Controls

| Input | Action |
|-------|--------|
| Mouse drag | Orbit camera |
| Scroll wheel | Zoom |
| W/A/S/D | Move camera target |
| Escape | Quit |

The ImGui panel exposes sample count (log scale, 1 to 1M), step size, march distance, distribution mean/variance, kernel selection, and dot color.

## Building

### Prerequisites

- CMake 3.25+
- A C++20 compiler (GCC 12+, Clang 15+)
- OpenGL 4.5 capable GPU and drivers
- Python 3 with `jinja2` installed (for GLAD code generation)
- System packages for GLFW: `libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config`

### Setup

Clone with vcpkg:

```bash
git clone <repo-url> statviz
cd statviz
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
```

### Build and Run

```bash
./build.sh          # Release build in ./build
./run.sh            # Launch
```

For a debug build:

```bash
./build.sh build Debug
```

### Tests

```bash
cd build && ctest --output-on-failure
```

Tests cover camera matrix math, CPU-reference Bernoulli sampling statistics, and fixed-point normalization precision.

## Tech Stack

| Component | Choice | Role |
|-----------|--------|------|
| GLFW | vcpkg | Window management and input |
| GLAD v2 | FetchContent | OpenGL 4.5 core loader |
| GLM | FetchContent | Vector/matrix math |
| ImGui | FetchContent | Immediate-mode UI |
| spdlog | vcpkg | Logging |
| GTest | vcpkg | Unit tests |

The build system uses CMake with vcpkg for system libraries and FetchContent for header-heavy dependencies. Shaders are copied to the build directory via a stamp-file target.

## Project Structure

```
src/
  main.cpp              Entry point, GLFW setup, input handling, main loop
  app/
    Shader.hpp/cpp       Shader loading (vertex/fragment and compute)
    Camera.hpp/cpp       Spherical orbit camera with WASD translation
    Renderer.hpp/cpp     GPU resource management, 3-pass pipeline
    Grid.hpp/cpp         XZ-plane reference grid
    UI.hpp/cpp           ImGui controls
  shaders/
    fullscreen.vert      Passthrough NDC quad
    raymarch.frag        Density ray marching with adaptive step size
    sample.comp          Atomic reduction + Bernoulli sampling
    display.frag         Binary texture compositing
    grid.vert/frag       Grid rendering
  common/
    Config.hpp           Distribution and render parameter structs
tests/
    test_camera.cpp      View/projection correctness, inverse roundtrip
    test_sampling.cpp    Bernoulli sampling statistics, PCG hash quality
    test_normalization.cpp  Fixed-point precision and overflow safety
```

## Future Work

- **Density-mapped coloring:** Use accumulated probability or local density gradient to drive a color ramp, adding a visual dimension beyond dot presence.
- **Custom kernels:** Runtime GLSL injection or a small expression language for user-defined density functions.
- **Isosurface overlay:** Optional marching-cubes mesh at a configurable density threshold for structural reference.
- **Multi-distribution compositing:** Layer multiple independent distributions with separate colors and blend modes.
- **Performance:** Hierarchical empty-space skipping, occupancy grid, or distance field acceleration for complex kernels.
- **Export:** Snapshot high-resolution frames or accumulate samples over many frames for publication-quality output.

## License

MIT
