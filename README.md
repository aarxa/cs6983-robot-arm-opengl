# CS6983 Coding Assignment #2 — Robot Arm in OpenGL

A simple robot arm modeled in Blender, loaded from `.obj` and rendered in OpenGL with a Blinn-Phong shader:

C = (ka·Ia + kd·max(N·L, 0))·Il + ks·Il·max(N·H, 0)^α

## Files

- `blender/robot_arm.blend` — Blender model
- `models/robot_arm.obj`, `models/robot_arm.mtl` — exported mesh and material colors (kd)
- `shaders/blinn_phong_vert.glsl`, `shaders/blinn_phong_frag.glsl` — Blinn-Phong shaders
- `shaders/simple_vert.glsl`, `shaders/simple_frag.glsl` — flat-color shaders for the grid
- `src/main.cpp` — OBJ loading, lighting uniforms, scene

## Build and run (from the project root)

macOS:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/cs6983_opengl_intro
```

Windows:

```
cmake -S . -B build
cmake --build build --config Release
.\build\Release\cs6983_opengl_intro.exe
```

GLFW and GLM are downloaded during the first configure.

Controls: drag or arrow keys to orbit, scroll or `-`/`=` to zoom, Esc to quit.
