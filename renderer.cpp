#include "renderer.h"
#include <cstdio>
#include <unordered_map>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "joypad.h"

namespace {

constexpr int window_width = 512 + 512 + 256 + 16 * 3;
constexpr int window_height = 480 + 256 + (16 + 16 + 3) * 2;
constexpr int texture_size = 1024;

const char* vertex_shader_data =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
#else
    "#version 330 core\n"
#endif
    "layout(location=0) in vec3 a_pos;\n"
    "layout(location=1) in vec2 a_tex;\n"
    "out vec2 v_tex;\n"
    "uniform mat4 transform;\n"
    "void main() {\n"
    "   gl_Position = transform * vec4(a_pos.x, a_pos.y, a_pos.z, 1.0);\n"
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

float transform_matrix[][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
};

uint8_t nametable_pixels[480][512][3];
uint8_t pattern_table_pixels[128][256][3];

std::unordered_map<int, Button> button_mapping{
    {GLFW_KEY_Z, Button::A},         {GLFW_KEY_X, Button::B},
    {GLFW_KEY_ENTER, Button::Start}, {GLFW_KEY_SPACE, Button::Select},
    {GLFW_KEY_UP, Button::Up},       {GLFW_KEY_DOWN, Button::Down},
    {GLFW_KEY_LEFT, Button::Left},   {GLFW_KEY_RIGHT, Button::Right},
};

ImVec2 uv(float px, float py) {
  return ImVec2(px / texture_size, py / texture_size);
}

}  // namespace

void Renderer::render() {
  glfwPollEvents();

  // Update texture from NES data
  update_texture();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  // Main screen
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  if (ImGui::Begin("NES Screen", nullptr, window_flags)) {
    ImGui::Image((ImTextureID)texture, ImVec2(512, 480), uv(0, 0),
                 uv(256, 240));
  }
  ImGui::End();

  // Controls
  ImGui::SetNextWindowPos(ImVec2(512 + 512 + 16 * 2, 0), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(256 + 16, window_height));
  if (ImGui::Begin("Help", nullptr, window_flags)) {
    ImGui::Text("Drag and drop to load a ROM file");
    ImGui::Text("");
    ImGui::Text("Controls:");
    ImGui::BulletText("[Arrow Keys] - DPAD");
    ImGui::BulletText("[Z] - A");
    ImGui::BulletText("[X] - B");
    ImGui::BulletText("[Enter] - Start");
    ImGui::BulletText("[Space] - Select");
  }
  ImGui::End();

  // Nametables
  ImGui::SetNextWindowPos(ImVec2(512 + 16, 0), ImGuiCond_Once);
  if (ImGui::Begin("PPU Name Tables", nullptr, window_flags)) {
    ImGui::Image((ImTextureID)texture, ImVec2(512, 480), uv(256, 0),
                 uv((256 + 512), 480));
  }
  ImGui::End();

  // Pattern tables
  ImGui::SetNextWindowPos(ImVec2(512 + 16, 480 + 16 + 16 + 3), ImGuiCond_Once);
  if (ImGui::Begin("PPU Pattern Tables", nullptr, window_flags)) {
    ImGui::Image((ImTextureID)texture, ImVec2(512, 256), uv(0, 256),
                 uv(256, 256 + 128));
  }
  ImGui::End();

  // Audio
  ImGui::SetNextWindowPos(ImVec2(0, 480 + 16 + 16 + 3), ImGuiCond_Once);
  ImGui::SetNextWindowContentSize(ImVec2(512, 256));
  if (ImGui::Begin("Audio Channels", nullptr, window_flags)) {
    ImVec2 plot_size(425.0f, 45.0f);
    const char* channel_names[5] = {"Pulse 1", "Pulse 2", "Triangle", "Noise",
                                    "DMC"};
    for (int i = 0; i < 5; i++) {
      auto& waveform = nes.apu.debug_waveforms[i];
      ImGui::PlotLines(channel_names[i], waveform.output_buffer,
                       waveform.buffer_size, 0, nullptr, 0, 1.0f, plot_size);
    }
  }
  ImGui::End();

  glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // TODO: Remove manual quad drawing (using imgui now)
  glUniformMatrix4fv(transform_loc, 1, GL_TRUE, &transform_matrix[0][0]);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);
}

bool Renderer::init() {
  if (!glfwInit()) {
    return false;
  }
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(window_width, window_height, "nes-emu", nullptr,
                            nullptr);
  if (!window) {
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowUserPointer(window, this);
  auto key_callback = [](GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
    static_cast<Renderer*>(glfwGetWindowUserPointer(window))
        ->key_callback(key, scancode, action, mods);
  };
  auto drop_callback = [](GLFWwindow* window, int count, const char** paths) {
    static_cast<Renderer*>(glfwGetWindowUserPointer(window))
        ->drop_callback(count, paths);
  };
  glfwSetKeyCallback(window, key_callback);
  glfwSetDropCallback(window, drop_callback);

#ifndef EMSCRIPTEN
  int version = gladLoadGL(glfwGetProcAddress);
  printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version),
         GLAD_VERSION_MINOR(version));
#endif

  // Setup imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  // Setup transform matrix so top-left is (0, 0) and bottom-right is
  // (window_width, window_height)
  transform_matrix[0][0] = 2.0f / window_width;
  transform_matrix[0][3] = -1.0f;
  transform_matrix[1][1] = -2.0f / window_height;
  transform_matrix[1][3] = +1.0f;

  // Add main screen quad
  // add_quad(0, 0, 512, 480, 0, 0, 0.5f);

  // Vertices
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Texture
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_size, texture_size, 0, GL_RGB,
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
  transform_loc = glGetUniformLocation(shader_program, "transform");

  glBindVertexArray(VAO);
  return true;
}

void Renderer::add_quad(float x,
                        float y,
                        float w,
                        float h,
                        float u,
                        float v,
                        float uv_scale) {
  unsigned int i = vertices.size() / 5;
  u /= texture_size;
  v /= texture_size;
  float u2 = u + (w * uv_scale) / texture_size;
  float v2 = v + (h * uv_scale) / texture_size;

  vertices.insert(vertices.end(), {x, y, 0, u, v});            // bottom left
  vertices.insert(vertices.end(), {x + w, y, 0, u2, v});       // bottom right
  vertices.insert(vertices.end(), {x, y + h, 0, u, v2});       // top left
  vertices.insert(vertices.end(), {x + w, y + h, 0, u2, v2});  // top right

  indices.insert(indices.end(), {i, i + 1, i + 3});
  indices.insert(indices.end(), {i, i + 2, i + 3});
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

double Renderer::time() {
  return glfwGetTime();
}

void Renderer::set_pixels(uint8_t* pixels, int x, int y, int w, int h) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE,
                  pixels);
}

void Renderer::update_texture() {
  set_pixels(&nes.ppu.pixels[0][0][0], 0, 0, 256, 240);

  nes.ppu.render_nametables(nametable_pixels);
  set_pixels(&nametable_pixels[0][0][0], 256, 0, 512, 480);

  nes.ppu.render_pattern_tables(pattern_table_pixels);
  set_pixels(&pattern_table_pixels[0][0][0], 0, 256, 256, 128);
}

void Renderer::key_callback(int key, int scancode, int action, int mods) {
  // TODO: Move out of renderer?
  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    auto it = button_mapping.find(key);
    if (it != button_mapping.end()) {
      Button button = it->second;
      nes.joypad.set_button_state(0, button, action == GLFW_PRESS);
    }
  }
}

void Renderer::drop_callback(int count, const char** paths) {
  nes.load(paths[0]);
}