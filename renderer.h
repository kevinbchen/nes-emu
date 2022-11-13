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
  Renderer(NES& nes, int window_width, int window_height, int texture_size)
      : nes(nes),
        window_width(window_width),
        window_height(window_height),
        texture_size(texture_size) {}

  bool init();
  void destroy();
  void render();

  bool done();

  // Add a quad to draw; call before init().
  // x, y, w, h, u, v are in pixel units
  void add_quad(float x,
                float y,
                float w,
                float h,
                float u,
                float v,
                float uv_scale = 1.0f);

  // Expects a 240x256x3 buffer
  void set_pixels(uint8_t* pixels, int x, int y, int w, int h);

 private:
  NES& nes;

  GLFWwindow* window = nullptr;
  int window_width;
  int window_height;
  int texture_size;

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

  void key_callback(int key, int scancode, int action, int mods);
  void drop_callback(int count, const char** paths);
};
