#pragma once
#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif
#include <GLFW/glfw3.h>

class Renderer {
 public:
  Renderer() = default;

  bool init();
  void destroy();
  void render();

  bool done();

  // Expects a 240x256x3 buffer
  void set_pixels(uint8_t* pixels);

 private:
  GLFWwindow* window = nullptr;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
  unsigned int texture;

  unsigned int vertex_shader;
  unsigned int fragment_shader;
  unsigned int shader_program;
};
