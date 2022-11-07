#include "renderer.h"
#include <cstdio>

namespace {

static const int SCREEN_WIDTH = 256;
static const int SCREEN_HEIGHT = 240;
static const int TEXTURE_SIZE = SCREEN_WIDTH;

const char* vertex_shader_data =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
#else
    "#version 330 core\n"
#endif
    "layout(location=0) in vec3 a_pos;\n"
    "layout(location=1) in vec2 a_tex;\n"
    "out vec2 v_tex;\n"
    "void main() {\n"
    "   gl_Position = vec4(a_pos.x, a_pos.y, a_pos.z, 1.0);\n"
    "   v_tex = a_tex;\n"
    "}";

const char* fragment_shader_data =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
    "precision mediump float;\n"
#else
    "#version 330 core\n"
#endif
    "uniform sampler2D u_texture;\n"
    "in vec2 v_tex;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "   vec4 color = texture(u_texture, v_tex);\n"
    "   o_color = color;\n"
    "}";

// Fullscreen rectangle
constexpr float v_max = (float)SCREEN_HEIGHT / TEXTURE_SIZE;
float vertices[] = {
    // x, y, z, u, v
    -1.0f, -1.0f, 0.0f, 0.0f, v_max,  // bottom left
    1.0f,  -1.0f, 0.0f, 1.0f, v_max,  // bottom right
    -1.0f, 1.0f,  0.0f, 0.0f, 0.0f,   // top left
    1.0f,  1.0f,  0.0f, 1.0f, 0.0f    // top right
};
unsigned int indices[] = {
    0, 1, 3,  //
    0, 2, 3   //
};

}  // namespace

void Renderer::render() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glfwSwapBuffers(window);
  glfwPollEvents();
}

bool Renderer::init() {
  if (!glfwInit()) {
    return false;
  }
  window = glfwCreateWindow(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, "nes-emu",
                            nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);

#ifndef EMSCRIPTEN
  int version = gladLoadGL(glfwGetProcAddress);
  printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version),
         GLAD_VERSION_MINOR(version));
#endif

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Texture
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Setup shaders
  int success;
  char log[512];

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_data, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, nullptr, log);
    printf("Error in vertex shader compilation\n%s\n", log);
    return false;
  }

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_data, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, nullptr, log);
    printf("Error in fragment shader compilation\n%s\n", log);
    return false;
  }

  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, nullptr, log);
    printf("Error linking shader program\n%s\n", log);
    return false;
  }
  glUseProgram(shader_program);

  glBindVertexArray(VAO);
  return true;
}

void Renderer::destroy() {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glfwDestroyWindow(window);
  glfwTerminate();
}

bool Renderer::done() {
  return glfwWindowShouldClose(window);
}

void Renderer::set_pixels(uint8_t* pixels) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_SIZE, SCREEN_HEIGHT, GL_RGB,
                  GL_UNSIGNED_BYTE, pixels);
}