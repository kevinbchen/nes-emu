#pragma once

#include <cstdint>

class Cartridge {
 public:
  uint8_t pgr_rom_banks[8][0x4000];  // 16kb each
  uint8_t chr_rom_banks[8][0x2000];  // 8kb each

  Cartridge() {}

  void load(const char* filename);
  uint8_t mem_read(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);

  uint8_t chr_mem_read(uint16_t addr);
  void chr_mem_write(uint16_t addr, uint8_t value);
};
