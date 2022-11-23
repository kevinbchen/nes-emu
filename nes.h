#pragma once
#include <cstdint>
#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu.h"

class NES {
 public:
  CPU cpu;
  PPU ppu;
  APU apu;
  Cartridge cartridge;
  Joypad joypad;
  bool loaded = false;

  NES() : cpu(*this), ppu(*this), apu(*this), cartridge(*this) {}
  void load(const char* filename);
  void run_frame();
};