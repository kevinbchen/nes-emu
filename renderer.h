#pragma once
#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <vector>
#include "nes.h"

class Renderer {
 public:
  Renderer(NES& nes) : nes(nes) {}

  bool init();
  void destroy();
  void render();
  bool done();

 private:
  NES& nes;

  GLFWwindow* window = nullptr;

  unsigned int VAO;
  unsigned int VBO;
  unsigned int EBO;
  unsigned int texture;
  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  unsigned int vertex_shader;
  unsigned int fragment_shader;
  unsigned int shader_program;
  unsigned int transform_loc;

  // x, y, w, h, u, v are in pixel units
  // Add a quad to draw; call during init() before vbo creation.
  void add_quad(float x,
                float y,
                float w,
                float h,
                float u,
                float v,
                float uv_scale = 1.0f);
  void set_pixels(uint8_t* pixels, int x, int y, int w, int h);
  void update_texture();

  void key_callback(int key, int scancode, int action, int mods);
  void drop_callback(int count, const char** paths);
};
