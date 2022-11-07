#include "cartridge.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

void Cartridge::load(const char* filename) {
  printf("Loading %s...\n", filename);
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not load cartridge file");
  }

  char header[16];
  file.read(header, 16);
  if (!(header[0] == 'N' && header[1] == 'E' && header[2] == 'S' &&
        header[3] == 0x1A)) {
    throw std::runtime_error("Invalid cartridge file");
  }
  int num_pgr_banks = header[4];
  int num_chr_banks = header[5];
  uint8_t rom_ctrl1 = header[6];
  uint8_t rom_ctrl2 = header[7];
  int num_ram_banks = std::max(1, (int)header[8]);
  printf("PGR banks: %d\tCHR banks: %d\n", num_pgr_banks, num_chr_banks);

  bool has_trainer = rom_ctrl1 & 0x04;
  bool has_ram = rom_ctrl1 & 0x02;

  if (has_trainer) {
    // TODO
    throw std::runtime_error("Trainer unhandled");
  }
  for (int i = 0; i < num_pgr_banks; i++) {
    file.read(reinterpret_cast<char*>(&pgr_rom_banks[i]), 0x4000);  // 16kb
  }

  for (int i = 0; i < num_chr_banks; i++) {
    file.read(reinterpret_cast<char*>(&chr_rom_banks[i]), 0x2000);  // 8kb
  }
}

uint8_t Cartridge::mem_read(uint16_t addr) {
  if (addr >= 0x8000) {
    return pgr_rom_banks[0][addr & 0x3FFF];
  }
  return 0;
}

void Cartridge::mem_write(uint16_t addr, uint8_t value) {}

uint8_t Cartridge::chr_mem_read(uint16_t addr) {
  return chr_rom_banks[0][addr & 0x1FFF];
}

void Cartridge::chr_mem_write(uint16_t addr, uint8_t value) {}