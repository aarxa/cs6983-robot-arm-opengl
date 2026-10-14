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

## Reflection

### 1. What was the most difficult part of this project, and how did you get past it?

The most difficult part was implementing the Blinn-Phong equation and actually navigating OpenGL to understand what everything was doing. I used AI to help write the shader and loader code, so my real challenge was understanding how all the pieces connected, like how `main.cpp` sends values such as the light position and material color to the shaders as uniforms. To get past it, I went through the shaders line by line and matched each part of the code to a term in the equation. I also ran into smaller problems, like my colors not showing in Blender because the viewport was in Solid mode, and the model getting cut off when I zoomed out because the camera's far plane was set too close. Working through these helped me understand how the parts of the pipeline fit together.

### 2. What was your biggest learning outcome?

My biggest learning outcome was learning to navigate OpenGL and exploring Blender, since until now I had only used Maya. Building the robot arm out of simple shapes in Blender showed me how a model is exported as an `.obj` file, which stores the vertices and normals, along with an `.mtl` file that stores the material colors. In OpenGL, I learned how that data is loaded onto the GPU and drawn with shaders. I also learned how each part of the Blinn-Phong equation affects the final look, like how the diffuse term makes the sides facing away from the light darker and the specular term creates the shiny highlights. Seeing my own Blender colors show up in the OpenGL window as the kd value helped me connect the modeling side to the rendering side.

### 3. What does the vertex shader do? What does the fragment shader do?

The vertex shader runs once for every vertex of the model and places it in 3D space on the screen. It takes in each vertex's position and normal, then uses the model matrix and the view-projection matrix to transform it into its final screen position (`gl_Position`). It also passes the world position and the normal on to the fragment shader for lighting. Between the two shaders, the GPU rasterizes the triangles, which fills in the space between the vertices with fragments. The fragment shader then runs on each fragment and calculates its final color using the Blinn-Phong equation, with the normal (N), the direction to the light (L) and the half-vector (H). Depth is not calculated in my fragment shader; OpenGL's depth test uses the vertex positions to decide which surface is in front.

### 4. How does this project relate to your knowledge of Physical AI?

In this course we are exploring how to navigate and work in 3D spaces, and presenting that space on a platform like OpenGL is one way to do it. A robot arm is a physical object, and to build an AI that controls one, we first need a way to represent its parts and their positions in 3D. In this project I used transforms like translation, rotation and scale to place the arm in the scene, which is how a robot's position and movement can be described. Rendering it with lighting makes the arm look like a real object instead of flat shapes. Since the assignment says we are not adding bones yet, I assume a future step will be rigging the joints so the arm can move, which brings it closer to how a real robot works.
