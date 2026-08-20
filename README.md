# 🌌 Relativistic Raytracer - Vulkan Implementation

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.4.335.0-red.svg)](https://vulkan.lunarg.com/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)](https://github.com/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)

## 📋 Project Overview

This project implements a **real-time relativistic raytracer** using **Vulkan Compute Shaders**, developed as part of a thesis work and presented at SIBGRAPI 2026. The system simulates light propagation in gravitational fields, enabling visualization of relativistic phenomena such as gravitational lensing, redshift, and spacetime rotation effects.

### 🎯 Key Features

- **Three Physical Metrics**: Newton, Schwarzschild, and Kerr
- **Two Numerical Integrators**: Euler and Runge-Kutta 4th order (RK4)
- **Real-Time Rendering**: GPU implementation via Vulkan Compute Shaders
- **Automated Benchmark System**: Performance metrics (FPS) and visual quality collection
- **Multiple Scenes**: From simple black holes to complete planetary systems
- **Cross-Platform**: Windows and Linux support

## 🏗️ Architecture

### Project Structure
```
Project-RayTracing-Vulkan/
├── src/                    # C++ source code
│   ├── main.cpp           # Entry point
│   ├── game.cpp           # Main engine (119KB)
│   ├── game.h             # Engine interfaces
│   ├── utils.cpp          # Vulkan utilities
│   └── utils.h            # Debug headers
├── shaders/               # GLSL shaders
│   └── shader.comp        # Main compute shader (16KB)
├── textures/              # Visual assets
│   ├── 2k_*.jpg          # 2K planetary textures
│   └── *.png             # Skyboxes and cubemaps
├── models/                # 3D models (optional)
├── CMakeLists.txt         # Build system
└── vcpkg.json            # Dependencies
```

### Rendering Pipeline

1. **Vulkan Initialization**: Device, queues and compute pipeline setup
2. **Camera System**: Viewport and projection management
3. **Geodesic Integration**: Numerical integration of motion equations
4. **Ray Tracing**: Intersection calculations in curved spacetime  
5. **Shading**: Planetary textures and skyboxes application
6. **Benchmark System**: Automated metrics collection

### Metric Implementation

#### 🍎 Newton (Euclidean Metric)
```glsl
// Classical gravitation
accel += toSphere * (sphereRadiusAdjusted * 0.5) / d2_3;
```

#### ⚫ Schwarzschild (Static Black Hole)
```glsl
// Relativistic deflection
vec3 h = cross(-toSphere, v);
accel += toSphere * (1.5 * sphereRadiusAdjusted * dot(h, h)) / d2_5;
```

#### 🌀 Kerr (Rotating Black Hole)
```glsl
// Frame-dragging + deflection
vec3 a_schwarzschild = -r_vec * (1.5 * sphereRadiusAdjusted * dot(h, h)) / d2_5;
vec3 spin_vec = KERR_SPIN_AXIS * sphereRadiusAdjusted * cam.spin_speed;
vec3 H = (2.0 / d2_5) * (3.0 * r_vec * dot(spin_vec, r_vec) - spin_vec * d2_2);
vec3 a_frame_drag = -cross(v, H);
accel += a_schwarzschild + a_frame_drag;
```

### Integration System

#### ⚡ Euler (1st Order)
```glsl
void step_euler(inout Ray ray, float h, SceneConfig scene) {
    vec3 accel = get_gravity_accel(ray.origin, ray.direction, scene);
    ray.direction = normalize(ray.direction + accel * h);
    ray.origin += ray.direction * h;
}
```

#### 🎯 Runge-Kutta 4 (4th Order)
```glsl
void step_rk4(inout Ray ray, float h, SceneConfig scene) {
    // Classical RK4 implementation with 4 evaluations per step
    vec3 k1_v = get_gravity_accel(p, v, scene);
    vec3 k2_v = get_gravity_accel(p + 0.5*h*k1_p, v + 0.5*h*k1_v, scene);
    vec3 k3_v = get_gravity_accel(p + 0.5*h*k2_p, v + 0.5*h*k2_v, scene);
    vec3 k4_v = get_gravity_accel(p + h*k3_p, v + h*k3_v, scene);
    // Weighted combination of gradients
}
```

## 📊 Key Results

### Performance Benchmark

The automated benchmark system collects detailed metrics across multiple configurations:

- **Resolutions**: 144p, 480p, 720p, 1080p
- **Metrics**: Newton, Schwarzschild, Kerr  
- **Integrators**: Euler vs RK4
- **Scenes**: 6 different scenarios (planets, multi-body systems, etc.)

#### 📈 Performance Cross-over

| Resolution | Newton (Euler) | Schwarzschild (RK4) | Kerr (RK4) |
|-----------|---------------|-------------------|------------|
| 144p      | ~1200 FPS     | ~400 FPS          | ~300 FPS   |
| 720p      | ~300 FPS      | ~100 FPS          | ~75 FPS    |
| 1080p     | ~180 FPS      | ~60 FPS           | ~45 FPS    |

#### 🎯 Gravitational Mass Limits

- **Schwarzschild Radius**: RS = 2GM/c² ≈ 29.5 km (for 10 solar masses)
- **Event Horizon**: Automatic detection and silhouette rendering
- **Ergosphere Region**: Frame-dragging simulation in Kerr metrics

### Visual Quality

The system implements visual validation through:

- **Planetary Textures**: High-resolution spherical mapping (2K)
- **Dynamic Skyboxes**: Cubemaps for different environments
- **Relativistic Effects**: Gravitational lensing, aberration, Doppler shift

## 🚀 How to Run

### Prerequisites

- **Windows 10/11** or **Ubuntu 20.04+**
- **Vulkan SDK 1.4.335.0** or higher
- **CMake 3.20** or higher
- **Visual Studio 2019+** (Windows) or **GCC 9+** (Linux)
- **vcpkg** (package manager)

### Dependencies

```json
{
  "name": "vulkangame",
  "version": "1.0.0",
  "dependencies": [
    "glfw3",    // Window system
    "glm",      // 3D mathematics
    "stb"       // Image loading
  ]
}
```

### Installation

#### Windows
```bash
# 1. Clone the repository
git clone https://github.com/your-username/Project-RayTracing-Vulkan.git
cd Project-RayTracing-Vulkan

# 2. Configure vcpkg (if not already configured)
vcpkg install glfw3:x64-windows glm:x64-windows stb:x64-windows

# 3. Configure the project
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake

# 4. Compile
cmake --build build --config Release

# 5. Run
.\build\bin\Release\VulkanGame.exe
```

#### Linux
```bash
# 1. Install system dependencies
sudo apt update
sudo apt install cmake build-essential libvulkan-dev vulkan-utils

# 2. Clone and configure
git clone https://github.com/your-username/Project-RayTracing-Vulkan.git
cd Project-RayTracing-Vulkan

# 3. Configure vcpkg
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install glfw3 glm stb

# 4. Build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release -j$(nproc)

# 5. Run
./build/bin/VulkanGame
```

### Controls

| Key | Function |
|-----|----------|
| `W/A/S/D` | Camera movement |
| `Mouse` | Camera rotation |
| `R` | Toggle relativistic mode |
| `M` | Switch metric (Newton→Schwarzschild→Kerr) |
| `I` | Switch integrator (Euler↔RK4) |
| `L` | Toggle FPS display |
| `P` | Screenshot capture |
| `1-6` | Select scene |

### Benchmark Mode

To run automated benchmarks, uncomment the relevant lines in `game.cpp` in the `mainLoop()` function. The system will generate:

- **Automatic screenshots** in `Benchmarks/`
- **CSV data** with detailed metrics
- **Performance comparisons** between configurations

## 📚 Academic References

This project is grounded in the scientific literature of general relativity and numerical methods:

1. **Schwarzschild, K.** (1916). "Über das Gravitationsfeld eines Massenpunktes nach der Einsteinschen Theorie"
2. **Kerr, R.P.** (1963). "Gravitational field of a spinning mass as an example of algebraically special metrics"  
3. **Chandrasekhar, S.** (1983). "The Mathematical Theory of Black Holes"
4. **Marck, J.A.** (1996). "Short-cut method of solution of geodesic equations for Schwarzschild black hole"
5. **James, O., von Tunzelmann, E., Franklin, P., Thorne, K.S.** (2015). "Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar"

## 🤝 Contributions

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/new-metric`)
3. Commit your changes (`git commit -am 'Add Reissner-Nordström metric'`)
4. Push to the branch (`git push origin feature/new-metric`)
5. Open a Pull Request

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Authors

**Artur Raffael Cavalcanti** and **Lucas Silva Figueiredo**  
*Universidade Federal Rural de Pernambuco (UFRPE), Recife, Brazil*

---

*"In space-time, there are no preferred frames of reference, only beautiful mathematics."* - Albert Einstein (paraphrased)