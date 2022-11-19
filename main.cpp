#include <chrono>
#include <cstdio>
#include <stdexcept>
#include "audio.h"
#include "nes.h"
#include "renderer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

NES nes;
Renderer renderer(nes);
Audio audio(nes);

void loop() {
  while (true) {
    nes.cpu.execute();
    if (nes.ppu.frame_ready) {
      nes.ppu.frame_ready = false;
      break;
    }
  }
  audio.output();
  renderer.render();
  nes.ppu.clear_pixels();
}

int main(int argc, char* agv[]) {
  try {
    if (!renderer.init()) {
      return -1;
    }
    if (!audio.init()) {
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