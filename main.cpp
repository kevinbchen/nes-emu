#include <chrono>
#include <cstdio>
#include <stdexcept>
#include "nes.h"
#include "renderer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

constexpr int screen_width = 256;
constexpr int screen_height = 240;

NES nes;
Renderer renderer(nes, 256 * 2 + 8 + 512, 240 * 2, 1024);

uint8_t nametable_pixels[480][512][3];

void loop() {
  while (true) {
    nes.cpu.execute();
    if (nes.ppu.frame_ready) {
      nes.ppu.frame_ready = false;
      break;
    }
  }
  renderer.set_pixels(&nes.ppu.pixels[0][0][0], 0, 0, screen_width,
                      screen_height);
  nes.ppu.render_nametables(nametable_pixels);
  renderer.set_pixels(&nametable_pixels[0][0][0], 256, 0, 512, 480);
  renderer.render();
}

int main(int argc, char* agv[]) {
  try {
    renderer.add_quad(0, 0, screen_width * 2, screen_height * 2, 0, 0, 0.5f);
    renderer.add_quad(512 + 8, 0, 512, 480, 256, 0);

    if (!renderer.init()) {
      return -1;
    }

    nes.load("roms/nestest.nes");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(&loop, 0, 1);
#else
    while (!renderer.done()) {
      loop();
    }
#endif

    renderer.destroy();
  } catch (std::runtime_error& e) {
    printf("%s\n", e.what());
  }
  return 0;
}