#pragma once

// Thanks CursorAI. The driver, not the linker, provides OpenGL entry points. After
// glfwMakeContextCurrent, glfwGetProcAddress asks the driver for each
// function this demo actually calls. A larger codebase uses GLAD for
// the same step across the whole API.

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstddef>
#include <iostream>

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLint = int;
using GLuint = unsigned int;
using GLsizei = int;
using GLsizeiptr = std::ptrdiff_t;
using GLfloat = float;
using GLchar = char;

#if defined(_WIN32) && !defined(_WIN64)
#define CS6983_APIENTRY __stdcall
#else
#define CS6983_APIENTRY
#endif

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_LINES 0x0001
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_TEST 0x0B71
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FLOAT 0x1406
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82

#define CS6983_GL_FUNC(ret, name, ...) \
    inline ret(CS6983_APIENTRY* name)(__VA_ARGS__) = nullptr

CS6983_GL_FUNC(void, glClear, GLbitfield);
CS6983_GL_FUNC(void, glClearColor, GLfloat, GLfloat, GLfloat, GLfloat);
CS6983_GL_FUNC(void, glViewport, GLint, GLint, GLsizei, GLsizei);
CS6983_GL_FUNC(void, glEnable, GLenum);
CS6983_GL_FUNC(void, glDrawArrays, GLenum, GLint, GLsizei);
CS6983_GL_FUNC(void, glGenBuffers, GLsizei, GLuint*);
CS6983_GL_FUNC(void, glBindBuffer, GLenum, GLuint);
CS6983_GL_FUNC(void, glBufferData, GLenum, GLsizeiptr, const void*, GLenum);
CS6983_GL_FUNC(void, glGenVertexArrays, GLsizei, GLuint*);
CS6983_GL_FUNC(void, glBindVertexArray, GLuint);
CS6983_GL_FUNC(void, glEnableVertexAttribArray, GLuint);
CS6983_GL_FUNC(void, glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
CS6983_GL_FUNC(GLuint, glCreateShader, GLenum);
CS6983_GL_FUNC(void, glShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint*);
CS6983_GL_FUNC(void, glCompileShader, GLuint);
CS6983_GL_FUNC(void, glGetShaderiv, GLuint, GLenum, GLint*);
CS6983_GL_FUNC(void, glGetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*);
CS6983_GL_FUNC(void, glDeleteShader, GLuint);
CS6983_GL_FUNC(GLuint, glCreateProgram);
CS6983_GL_FUNC(void, glAttachShader, GLuint, GLuint);
CS6983_GL_FUNC(void, glLinkProgram, GLuint);
CS6983_GL_FUNC(void, glGetProgramiv, GLuint, GLenum, GLint*);
CS6983_GL_FUNC(void, glGetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*);
CS6983_GL_FUNC(void, glUseProgram, GLuint);
CS6983_GL_FUNC(GLint, glGetUniformLocation, GLuint, const GLchar*);
CS6983_GL_FUNC(void, glUniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*);
CS6983_GL_FUNC(void, glUniform3f, GLint, GLfloat, GLfloat, GLfloat);
CS6983_GL_FUNC(void, glUniform1f, GLint, GLfloat);
CS6983_GL_FUNC(void, glUniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*);

inline bool load_gl() {
    int missing = 0;
#define CS6983_LOAD(fn)                                                         \
    fn = reinterpret_cast<decltype(fn)>(glfwGetProcAddress(#fn));              \
    if (!fn) {                                                                  \
        std::cerr << "OpenGL entry point missing: " #fn "\n";                  \
        ++missing;                                                              \
    }
    CS6983_LOAD(glClear);
    CS6983_LOAD(glClearColor);
    CS6983_LOAD(glViewport);
    CS6983_LOAD(glEnable);
    CS6983_LOAD(glDrawArrays);
    CS6983_LOAD(glGenBuffers);
    CS6983_LOAD(glBindBuffer);
    CS6983_LOAD(glBufferData);
    CS6983_LOAD(glGenVertexArrays);
    CS6983_LOAD(glBindVertexArray);
    CS6983_LOAD(glEnableVertexAttribArray);
    CS6983_LOAD(glVertexAttribPointer);
    CS6983_LOAD(glCreateShader);
    CS6983_LOAD(glShaderSource);
    CS6983_LOAD(glCompileShader);
    CS6983_LOAD(glGetShaderiv);
    CS6983_LOAD(glGetShaderInfoLog);
    CS6983_LOAD(glDeleteShader);
    CS6983_LOAD(glCreateProgram);
    CS6983_LOAD(glAttachShader);
    CS6983_LOAD(glLinkProgram);
    CS6983_LOAD(glGetProgramiv);
    CS6983_LOAD(glGetProgramInfoLog);
    CS6983_LOAD(glUseProgram);
    CS6983_LOAD(glGetUniformLocation);
    CS6983_LOAD(glUniformMatrix4fv);
    CS6983_LOAD(glUniform3f);
    CS6983_LOAD(glUniform1f);
    CS6983_LOAD(glUniformMatrix3fv);
#undef CS6983_LOAD
    return missing == 0;
}
