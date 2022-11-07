#include <chrono>
#include <cstdio>
#include <stdexcept>
#include "nes.h"
#include "renderer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

NES nes;
Renderer renderer(nes);

void loop() {
  while (true) {
    nes.cpu.execute();
    // nes.cpu.print_state();
    if (nes.cpu.done) {
      exit(0);
    }
    if (nes.ppu.frame_ready) {
      nes.ppu.frame_ready = false;
      break;
    }
  }
  renderer.set_pixels(&nes.ppu.pixels[0][0][0]);
  /*
  for (int y = 0; y < 30; y++) {
    for (int x = 0; x < 32; x++) {
      int i = y * 32 + x;
      uint8_t c = nes.ppu.mem_read(0x2000 + i);
      printf("%02x ", c);
    }
    printf("\n");
  }
  printf("=========================================\n");
  */
  renderer.render();
}

int main(int argc, char* agv[]) {
  try {
    if (!renderer.init()) {
      return -1;
    }

    // nes.cartridge.load("color_test_src/color_test.nes");
    // nes.cartridge.load("roms/nestest.nes");
    nes.cartridge.load("roms/donkeykong.nes");
    nes.cpu.power_on();

    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        // nes.ppu.render_tile(y * 16 + x, x * 8, y * 8);
        // nes.ppu.render_tile(y * 16 + x + 256, x * 8 + 8 * 16, y * 8);
      }
    }
    renderer.set_pixels(&nes.ppu.pixels[0][0][0]);

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