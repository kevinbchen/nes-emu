#pragma once
#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif
#include <GLFW/glfw3.h>

#include "nes.h"

class Renderer {
 public:
  Renderer(NES& nes) : nes(nes) {}

  bool init();
  void destroy();
  void render();

  bool done();

  // Expects a 240x256x3 buffer
  void set_pixels(uint8_t* pixels);

 private:
  NES& nes;

  GLFWwindow* window = nullptr;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
  unsigned int texture;

  unsigned int vertex_shader;
  unsigned int fragment_shader;
  unsigned int shader_program;

  void key_callback(int key, int scancode, int action, int mods);
};
