#include "nes.h"

void NES::load(const char* filename) {
  loaded = cartridge.load(filename);
  if (loaded) {
    cpu.power_on();
    ppu.power_on();
    apu.power_on();
  }
}

void NES::run_frame() {
  ppu.clear_pixels();
  if (!loaded) {
    return;
  }
  while (!ppu.frame_ready) {
    // Note: Emulated PPU and APU ticks are driven by the CPU
    cpu.execute();
  }
  ppu.frame_ready = false;
}