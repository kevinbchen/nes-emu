#pragma once
#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vector>
#include "nes/nes.h"

struct InputBinding {
  const char* name;
  Button nes_button;
  int glfw_key;
};

class Renderer {
 public:
  Renderer(NES& nes) : nes(nes) {}
  bool init();
  void destroy();
  void render();
  bool done();
  double time();

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

  InputBinding input_bindings[(int)Button::Count] = {
      {"A", Button::A, GLFW_KEY_Z},
      {"B", Button::B, GLFW_KEY_X},
      {"Start", Button::Start, GLFW_KEY_ENTER},
      {"Select", Button::Select, GLFW_KEY_SPACE},
      {"Up", Button::Up, GLFW_KEY_UP},
      {"Down", Button::Down, GLFW_KEY_DOWN},
      {"Left", Button::Left, GLFW_KEY_LEFT},
      {"Right", Button::Right, GLFW_KEY_RIGHT},
  };
  std::unordered_map<int, InputBinding*> input_mapping;
  int remapping_binding = -1;

  void render_controls();
  void init_input_bindings();
  void key_callback(int key, int scancode, int action, int mods);
  void drop_callback(int count, const char** paths);
};
