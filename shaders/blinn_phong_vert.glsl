#version 330 core

// vertex shader: places each vertex on screen and hands world-space
// position and normal to the fragment shader for per-pixel lighting
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;        // object -> world
uniform mat4 uViewProj;     // world -> clip
uniform mat3 uNormalMatrix; // transpose(inverse(mat3(uModel))), keeps normals perpendicular under non-uniform scale

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    gl_Position = uViewProj * worldPos;
}
