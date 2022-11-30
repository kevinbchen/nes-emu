#pragma once
#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#include <emscripten/html5.h>
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
  int gamepad_button;
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

  // TODO: Move input stuff out of renderer?
#ifdef __EMSCRIPTEN__
  // Unfortunately, glfw's gamepad functionality isn't supported with
  // emscripten, so we need to use the html5 gamepad api instead.
  InputBinding input_bindings[(int)Button::Count] = {
      {"A", Button::A, GLFW_KEY_Z, 0},
      {"B", Button::B, GLFW_KEY_X, 1},
      {"Start", Button::Start, GLFW_KEY_ENTER, 9},
      {"Select", Button::Select, GLFW_KEY_SPACE, 8},
      {"Up", Button::Up, GLFW_KEY_UP, 12},
      {"Down", Button::Down, GLFW_KEY_DOWN, 13},
      {"Left", Button::Left, GLFW_KEY_LEFT, 14},
      {"Right", Button::Right, GLFW_KEY_RIGHT, 15},
  };
#else
  InputBinding input_bindings[(int)Button::Count] = {
      {"A", Button::A, GLFW_KEY_Z, GLFW_GAMEPAD_BUTTON_A},
      {"B", Button::B, GLFW_KEY_X, GLFW_GAMEPAD_BUTTON_B},
      {"Start", Button::Start, GLFW_KEY_ENTER, GLFW_GAMEPAD_BUTTON_START},
      {"Select", Button::Select, GLFW_KEY_SPACE, GLFW_GAMEPAD_BUTTON_BACK},
      {"Up", Button::Up, GLFW_KEY_UP, GLFW_GAMEPAD_BUTTON_DPAD_UP},
      {"Down", Button::Down, GLFW_KEY_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
      {"Left", Button::Left, GLFW_KEY_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
      {"Right", Button::Right, GLFW_KEY_RIGHT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
  };
#endif
  std::unordered_map<int, InputBinding*> input_mapping;
  int remapping_binding = -1;
  bool key_states[(int)Button::Count] = {false};
  bool gamepad_states[(int)Button::Count] = {false};

  void render_controls();
  void init_input_bindings();
  void poll_joystick();
  void set_joypad_state();
  void key_callback(int key, int scancode, int action, int mods);
  void drop_callback(int count, const char** paths);
};
