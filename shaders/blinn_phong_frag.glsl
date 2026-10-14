#version 330 core

// fragment shader: computes the color of each pixel with the Blinn-Phong model
//   C = (ka*Ia + kd*max(N.L, 0)) * Il + ks * Il * max(N.H, 0)^alpha
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightPos;   // light position (world space)
uniform vec3 uViewPos;    // camera position (world space)
uniform vec3 uLightColor; // Il: light color / intensity
uniform vec3 uAmbient;    // Ia: ambient light intensity
uniform float uKa;        // ka: ambient reflectance
uniform vec3 uKd;         // kd: diffuse reflectance (the model color)
uniform float uKs;        // ks: specular reflectance
uniform float uShininess; // alpha: specular exponent

out vec4 fragColor;

void main() {
    // interpolation across the triangle shortens normals, so renormalize
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos); // toward the light
    vec3 V = normalize(uViewPos - vWorldPos);  // toward the camera
    vec3 H = normalize(L + V);                 // half-vector between L and V

    float diffuse = max(dot(N, L), 0.0);
    // no highlight on faces turned away from the light
    float specular = diffuse > 0.0 ? pow(max(dot(N, H), 0.0), uShininess) : 0.0;

    vec3 color = (uKa * uAmbient + uKd * diffuse) * uLightColor
               + uKs * uLightColor * specular;

    fragColor = vec4(color, 1.0);
}
