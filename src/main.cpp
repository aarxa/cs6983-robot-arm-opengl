// CS 6983 Module 2 Week 3
// Sample OpenGL Program

#include "gl_loader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct App {
    // screen size
    int width = 1920;
    int height = 1080;
    // initial camera position
    float yaw = 0.7f;
    float pitch = 0.4f;
    float distance = 7.0f;
    // dragging state
    bool dragging = false;
    // last mouse position
    double lastX = 0.0;
    double lastY = 0.0;
};

static App gApp;

// flat-color program, used for the grid lines (lines have no normals to light)
static GLuint gProgram = 0;
static GLint gMvpLoc = -1;
static GLint gColorLoc = -1;

// Blinn-Phong program, used for the robot arm
struct PhongProgram {
    GLuint id = 0;
    GLint model = -1;
    GLint viewProj = -1;
    GLint normalMatrix = -1;
    GLint lightPos = -1;
    GLint viewPos = -1;
    GLint lightColor = -1;
    GLint ambient = -1;
    GLint ka = -1;
    GLint kd = -1;
    GLint ks = -1;
    GLint shininess = -1;
};
static PhongProgram gPhong;

// one draw call per material in the .obj, so each part keeps its own kd
struct SubMesh {
    GLint first = 0;
    GLsizei count = 0;
    glm::vec3 kd{0.8f};
};
struct Model {
    GLuint vao = 0;
    std::vector<SubMesh> parts;
    glm::mat4 fit{1.0f}; // centers the model on the grid and scales it to a fixed height
};
static Model gArm;

static GLuint gGridVao = 0;
static GLuint gGridVertexCount = 0;

// colors
const glm::vec3 LIGHT_GRAY(0.28f, 0.31f, 0.34f); // Gray
const glm::vec3 DARK_GRAY(0.11f, 0.12f, 0.14f); // Dark Gray
const glm::vec3 YELLOW(0.83f, 0.64f, 0.36f); // Yellow
const glm::vec3 BLUE(0.32f, 0.58f, 0.78f); // Blue
const glm::vec3 RED(0.80f, 0.36f, 0.36f); // Red

// Blinn-Phong lighting parameters
const glm::vec3 LIGHT_POS(4.0f, 6.0f, 3.0f);
const glm::vec3 LIGHT_COLOR(1.0f, 1.0f, 1.0f); // Il
const glm::vec3 AMBIENT(0.25f, 0.27f, 0.30f);  // Ia
const float KA = 0.6f;                         // ka
const float KS = 0.5f;                         // ks
const float SHININESS = 48.0f;                 // alpha

// ###########################################################################################
// Shaders
// ###########################################################################################

// helper function to read the text from a file
// Thanks Cursor AI
static std::string read_text(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open " + path);
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Thanks Cursor AI
static GLuint compile_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Shader compile failed");
    }
    return shader;
}

// Thanks Cursor AI
static GLuint link_program(const std::string& vertPath, const std::string& fragPath) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, read_text(vertPath));
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, read_text(fragPath));
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Program link failed");
    }
    return program;
}

// ###########################################################################################
// Meshes
// ###########################################################################################


static GLuint upload_positions(const std::vector<float>& positions) {
    GLuint vao = 0; // vertex array object: how to interpret the vertex data
    GLuint vbo = 0; // vertex buffer object: where to store the vertex data
    // generate the vertex array and buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    // bind the vertex array and buffer
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // upload the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(float)), positions.data(), GL_STATIC_DRAW);
    // set the vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    // enable the vertex attribute array
    glEnableVertexAttribArray(0);
    // return the vertex array
    return vao;
}

// ###########################################################################################
// OBJ loading
// ###########################################################################################

// reads "newmtl" / "Kd" pairs from a .mtl file; missing file just means default colors
static std::map<std::string, glm::vec3> load_mtl(const std::string& path) {
    std::map<std::string, glm::vec3> colors;
    std::ifstream in(path);
    std::string line, current;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "newmtl") {
            ls >> current;
        } else if (tag == "Kd" && !current.empty()) {
            glm::vec3 kd;
            ls >> kd.r >> kd.g >> kd.b;
            colors[current] = kd;
        }
    }
    return colors;
}

// obj indices are 1-based, negative ones count back from the end
static int resolve_index(int idx, std::size_t size) {
    return idx > 0 ? idx - 1 : static_cast<int>(size) + idx;
}

// loads positions, normals and materials from a Wavefront .obj
// faces with more than three corners are split into a triangle fan
// output is interleaved per vertex: px py pz nx ny nz
static Model load_obj(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open " + path);
    }
    const std::string dir = path.substr(0, path.find_last_of("/\\") + 1);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::map<std::string, glm::vec3> materials;
    std::vector<float> vertices;
    Model model;
    model.parts.push_back(SubMesh{});

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "v") {
            glm::vec3 p;
            ls >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vn") {
            glm::vec3 n;
            ls >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "mtllib") {
            std::string file;
            std::getline(ls >> std::ws, file);
            for (const auto& [name, kd] : load_mtl(dir + file)) {
                materials[name] = kd;
            }
        } else if (tag == "usemtl") {
            std::string name;
            ls >> name;
            SubMesh part;
            part.first = static_cast<GLint>(vertices.size() / 6);
            auto it = materials.find(name);
            if (it != materials.end()) {
                part.kd = it->second;
            }
            model.parts.push_back(part);
        } else if (tag == "f") {
            // each corner is v, v/vt, v//vn or v/vt/vn
            std::vector<int> pi, ni;
            std::string corner;
            while (ls >> corner) {
                const std::size_t s1 = corner.find('/');
                pi.push_back(resolve_index(std::stoi(corner.substr(0, s1)), positions.size()));
                int n = -1;
                if (s1 != std::string::npos) {
                    const std::size_t s2 = corner.find('/', s1 + 1);
                    if (s2 != std::string::npos && s2 + 1 < corner.size()) {
                        n = resolve_index(std::stoi(corner.substr(s2 + 1)), normals.size());
                    }
                }
                ni.push_back(n);
            }
            for (std::size_t k = 1; k + 1 < pi.size(); ++k) {
                const std::size_t tri[3] = {0, k, k + 1};
                // flat face normal for corners the file gives no normal
                const glm::vec3 faceN = glm::normalize(glm::cross(
                    positions[pi[k]] - positions[pi[0]], positions[pi[k + 1]] - positions[pi[0]]));
                for (std::size_t c : tri) {
                    const glm::vec3& p = positions[pi[c]];
                    const glm::vec3 n = ni[c] >= 0 ? normals[ni[c]] : faceN;
                    vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
                }
            }
        }
    }
    if (vertices.empty()) {
        throw std::runtime_error("No faces in " + path);
    }

    // close off each part and drop empty ones
    const GLint total = static_cast<GLint>(vertices.size() / 6);
    for (std::size_t i = 0; i < model.parts.size(); ++i) {
        const GLint next = i + 1 < model.parts.size() ? model.parts[i + 1].first : total;
        model.parts[i].count = next - model.parts[i].first;
    }
    model.parts.erase(std::remove_if(model.parts.begin(), model.parts.end(),
                                     [](const SubMesh& m) { return m.count == 0; }),
                      model.parts.end());

    // center on x/z, rest the base on y = 0, scale to 2 units tall
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (const glm::vec3& p : positions) {
        lo = glm::min(lo, p);
        hi = glm::max(hi, p);
    }
    const float height = std::max(hi.y - lo.y, 1e-6f);
    const float scale = 2.0f / height;
    model.fit = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    model.fit = glm::translate(model.fit, glm::vec3(-(lo.x + hi.x) * 0.5f, -lo.y, -(lo.z + hi.z) * 0.5f));

    // upload: attribute 0 = position, attribute 1 = normal
    GLuint vbo = 0;
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "loaded " << path << ": " << total / 3 << " triangles, "
              << model.parts.size() << " materials\n";
    return model;
}

// Thanks Cursor AI
static GLuint make_grid() {
    // create a vector to store the vertices
    std::vector<float> positions;
    // create the vertices of the grid
    for (int i = -4; i <= 4; ++i) {
        const float x = static_cast<float>(i);
        positions.insert(positions.end(), {x, 0.0f, -4.0f, x, 0.0f, 4.0f});
        positions.insert(positions.end(), {-4.0f, 0.0f, x, 4.0f, 0.0f, x});
    }
    // set the number of vertices
    gGridVertexCount = static_cast<GLuint>(positions.size() / 3);
    return upload_positions(positions);
}

static void draw_mesh(GLuint vao, GLuint count, GLenum mode, const glm::mat4& mvp, const glm::vec3& color) {
    glUseProgram(gProgram);
    // set the Model-View-Projection matrix
    // glUniformMatrix4fv(location, count, transpose, value)
    glUniformMatrix4fv(gMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    // pass color to the fragment shader
    glUniform3f(gColorLoc, color.r, color.g, color.b);
    // bind the vertex array
    glBindVertexArray(vao);
    // draw the vertices
    glDrawArrays(mode, 0, static_cast<GLint>(count));
}

// ###########################################################################################
// Transform
// ###########################################################################################

// Unity Transform: translate, then rotate, then scale. Scale applies first
// to the vertex because the vector sits on the right: M * p = T * R * S * p.
static glm::mat4 model_matrix(const glm::vec3& position, float radians, const glm::vec3& scale) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, radians, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);
    return model;
}

// ###########################################################################################
// Camera
// ###########################################################################################

// camera position in world space (also needed for the specular term)
static glm::vec3 camera_position() {
    const float cosPitch = std::cos(gApp.pitch);
    // eye position is calculated based on the pitch and yaw angles
    const glm::vec3 eye(
        gApp.distance * cosPitch * std::sin(gApp.yaw), // x coordinate
        gApp.distance * std::sin(gApp.pitch), // y coordinate
        gApp.distance * cosPitch * std::cos(gApp.yaw) // z coordinate
    );
    return eye;
}

// Unity Camera position and orientation
static glm::mat4 view_matrix() {
    // lookAt(camera position, target position, up vector)
    return glm::lookAt(camera_position(), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// Unity Camera field of view, aspect, near plane, far plane
static glm::mat4 projection_matrix() {
    // calculate the aspect ratio
    const float aspect = gApp.height > 0
        ? static_cast<float>(gApp.width) / static_cast<float>(gApp.height)
        : 1.0f;

    // 70 degrees is the camera field of view (FOV)
    float fov = 40.0f;      
    // set clipping planes
    // near plane must stay above 0. camera-space z = 0 cannot be projected
    float near = 0.1f;
    float far = 15.0f;
    return glm::perspective(glm::radians(fov), aspect, near, far);
}

// ###########################################################################################
// Scene
// ###########################################################################################

static void draw_model(const Model& model, const glm::mat4& modelMatrix, const glm::mat4& vp) {
    const glm::mat4 m = modelMatrix * model.fit;
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(m)));
    const glm::vec3 eye = camera_position();

    glUseProgram(gPhong.id);
    glUniformMatrix4fv(gPhong.model, 1, GL_FALSE, glm::value_ptr(m));
    glUniformMatrix4fv(gPhong.viewProj, 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix3fv(gPhong.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform3f(gPhong.lightPos, LIGHT_POS.x, LIGHT_POS.y, LIGHT_POS.z);
    glUniform3f(gPhong.viewPos, eye.x, eye.y, eye.z);
    glUniform3f(gPhong.lightColor, LIGHT_COLOR.r, LIGHT_COLOR.g, LIGHT_COLOR.b);
    glUniform3f(gPhong.ambient, AMBIENT.r, AMBIENT.g, AMBIENT.b);
    glUniform1f(gPhong.ka, KA);
    glUniform1f(gPhong.ks, KS);
    glUniform1f(gPhong.shininess, SHININESS);

    glBindVertexArray(model.vao);
    for (const SubMesh& part : model.parts) {
        // kd is the model color, so it changes per material
        glUniform3f(gPhong.kd, part.kd.r, part.kd.g, part.kd.b);
        glDrawArrays(GL_TRIANGLES, part.first, part.count);
    }
}

static void draw_scene(float time) {
    // calculate the view-projection matrix
    const glm::mat4 vp = projection_matrix() * view_matrix();

    // draw the grid using lines
    draw_mesh(gGridVao, gGridVertexCount, GL_LINES, vp, LIGHT_GRAY);

    // draw the robot arm, slowly turning so the highlights move across it
    const glm::mat4 armModel = model_matrix(glm::vec3(0.0f), time * 0.3f, glm::vec3(1.0f));
    draw_model(gArm, armModel, vp);
}

// ###########################################################################################
// Callbacks functions
// ###########################################################################################

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gApp.width = width;
    gApp.height = height;
}

// ###########################################################################################
// Input Callbacks
// ###########################################################################################

// Thanks Cursor AI
static void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT) {
        gApp.yaw -= 0.08f;
    } else if (key == GLFW_KEY_RIGHT) {
        gApp.yaw += 0.08f;
    } else if (key == GLFW_KEY_UP) {
        gApp.pitch += 0.06f;
    } else if (key == GLFW_KEY_DOWN) {
        gApp.pitch -= 0.06f;
    } else if (key == GLFW_KEY_MINUS) {
        gApp.distance += 0.3f;
    } else if (key == GLFW_KEY_EQUAL) {
        gApp.distance -= 0.3f;
    }
}

// Thanks Cursor AI
static void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        gApp.dragging = action == GLFW_PRESS;
    }
}

// Thanks Cursor AI
static void cursor_callback(GLFWwindow*, double x, double y) {
    if (gApp.dragging) {
        gApp.yaw += static_cast<float>(x - gApp.lastX) * 0.008f;
        gApp.pitch += static_cast<float>(y - gApp.lastY) * 0.008f;
    }
    gApp.lastX = x;
    gApp.lastY = y;
}

// Thanks Cursor AI
static void scroll_callback(GLFWwindow*, double, double yOffset) {
    gApp.distance -= static_cast<float>(yOffset) * 0.4f;
}

// ###########################################################################################
// Camera Clamping
// ###########################################################################################

// Thanks Cursor AI
static void clamp_camera() {
    if (gApp.pitch > 1.15f) gApp.pitch = 1.15f;
    if (gApp.pitch < -1.15f) gApp.pitch = -1.15f;
    if (gApp.distance < 2.0f) gApp.distance = 2.0f;
    if (gApp.distance > 18.0f) gApp.distance = 18.0f;
}

// ###########################################################################################
// Main
// ###########################################################################################

int main() {
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW " << code << ": " << description << "\n";
    });
    if (!glfwInit()) {
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // create the window
    GLFWwindow* window = glfwCreateWindow(gApp.width, gApp.height, "CS 6983 OpenGL", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    // make the window the current context so we can draw to it
    glfwMakeContextCurrent(window);
    // wait for the display to refresh before swapping the buffers
    glfwSwapInterval(1);
    // set the callback functions for the window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwGetFramebufferSize(window, &gApp.width, &gApp.height);

    // run the driver 
    if (!load_gl()) {
        glfwTerminate();
        return 1;
    }

    // link the shaders
    try {
        gProgram = link_program(std::string(SHADER_DIR) + "/simple_vert.glsl",
                                std::string(SHADER_DIR) + "/simple_frag.glsl");
        gPhong.id = link_program(std::string(SHADER_DIR) + "/blinn_phong_vert.glsl",
                                 std::string(SHADER_DIR) + "/blinn_phong_frag.glsl");
        gArm = load_obj(std::string(MODEL_DIR) + "/robot_arm.obj");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        glfwTerminate();
        return 1;
    }
    // get the location of the uniform variables
    gMvpLoc = glGetUniformLocation(gProgram, "uMVP");
    gColorLoc = glGetUniformLocation(gProgram, "uColor");
    gPhong.model = glGetUniformLocation(gPhong.id, "uModel");
    gPhong.viewProj = glGetUniformLocation(gPhong.id, "uViewProj");
    gPhong.normalMatrix = glGetUniformLocation(gPhong.id, "uNormalMatrix");
    gPhong.lightPos = glGetUniformLocation(gPhong.id, "uLightPos");
    gPhong.viewPos = glGetUniformLocation(gPhong.id, "uViewPos");
    gPhong.lightColor = glGetUniformLocation(gPhong.id, "uLightColor");
    gPhong.ambient = glGetUniformLocation(gPhong.id, "uAmbient");
    gPhong.ka = glGetUniformLocation(gPhong.id, "uKa");
    gPhong.kd = glGetUniformLocation(gPhong.id, "uKd");
    gPhong.ks = glGetUniformLocation(gPhong.id, "uKs");
    gPhong.shininess = glGetUniformLocation(gPhong.id, "uShininess");

    // make the meshes
    gGridVao = make_grid();
    // enable the depth test
    glEnable(GL_DEPTH_TEST);
    // set the background color
    glClearColor(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b, 1.0f);
    //glClearColor(RED.r, RED.g, RED.b, 1.0f);

    std::cout << "drag or arrows to orbit, wheel or -/= to zoom, Esc quits\n";
    // main loop
    while (!glfwWindowShouldClose(window)) {
        // poll for events (inputs)
        glfwPollEvents();
        // get the time
        const float time = static_cast<float>(glfwGetTime());
        // clamp the camera to prevent it from going too far
        clamp_camera();
        
        // set the viewport and clear the color and depth buffers to prepare for the next frame
        glViewport(0, 0, gApp.width, gApp.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the scene 
        draw_scene(time);
        // swap the buffers to display the new frame
        glfwSwapBuffers(window);
    }

    // terminate the window
    glfwTerminate();
    return 0;
}
