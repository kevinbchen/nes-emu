#pragma once

#include <cstdint>

class Cartridge {
 public:
  uint8_t pgr_rom_banks[8][0x4000];

  Cartridge() {}

  void load(const char* filename);
  uint8_t* mem(uint16_t addr);
};
