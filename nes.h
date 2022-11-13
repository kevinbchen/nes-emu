#pragma once
#include <cstdint>
#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu.h"

class NES {
 public:
  NES() : cpu(*this), ppu(*this) {}

  CPU cpu;
  PPU ppu;
  Cartridge cartridge;
  Joypad joypad;

  void load(const char* filename) {
    cartridge.load(filename);
    ppu.set_mirror_mode(cartridge.get_mirror_mode());
    cpu.power_on();
    ppu.power_on();
  }
};