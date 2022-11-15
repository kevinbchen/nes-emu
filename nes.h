#pragma once
#include <cstdint>
#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu.h"

class NES {
 public:
  NES() : cpu(*this), ppu(*this), apu(*this) {}

  CPU cpu;
  PPU ppu;
  APU apu;
  Cartridge cartridge;
  Joypad joypad;

  void load(const char* filename) {
    cartridge.load(filename);
    cpu.power_on();
    ppu.power_on();
    apu.power_on();
  }
};