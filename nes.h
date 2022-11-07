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
};