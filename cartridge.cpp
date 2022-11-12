#include "cartridge.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

Mapper::Mapper(char header[]) {
  num_pgr_banks = header[4];
  num_chr_banks = header[5];
  uint8_t rom_ctrl1 = header[6];
  uint8_t rom_ctrl2 = header[7];
  num_ram_banks = std::max(1, (int)header[8]);
  has_trainer = rom_ctrl1 & 0x04;
  has_ram = rom_ctrl1 & 0x02;
  vertical_mirroring = rom_ctrl1 & 0x01;

  printf("PGR banks: %d, CHR banks: %d\n", num_pgr_banks, num_chr_banks);
}

class Mapper0 : public Mapper {
 public:
  Mapper0(char header[]) : Mapper(header) {}

  uint8_t mem_read(uint16_t addr) override {
    if (addr < 0xC000) {
      return pgr_rom_banks[0][addr & 0x3FFF];
    } else {
      int i = num_pgr_banks == 1 ? 0 : 1;
      return pgr_rom_banks[i][addr & 0x3FFF];
    }
    return 0;
  }

  void mem_write(uint16_t addr, uint8_t value) override {}

  uint8_t chr_mem_read(uint16_t addr) override {
    return chr_rom_banks[0][addr & 0x1FFF];
  }

  void chr_mem_write(uint16_t addr, uint8_t value) override {
    chr_rom_banks[0][addr & 0x1FFF] = value;
  }
};

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

  uint8_t rom_ctrl1 = header[6];
  uint8_t rom_ctrl2 = header[7];
  int mapper_num = (rom_ctrl2 & 0xF0) | (rom_ctrl1 >> 4);
  printf("Mapper: %d\n", mapper_num);

  switch (mapper_num) {
    case 0:
      mapper = std::make_unique<Mapper0>(header);
      break;
    default:
      throw std::runtime_error("Unsupported mapper type " +
                               std::to_string(mapper_num));
  }

  if (mapper->has_trainer) {
    // TODO
    throw std::runtime_error("Trainer unhandled");
  }
  for (int i = 0; i < mapper->num_pgr_banks; i++) {
    file.read(reinterpret_cast<char*>(&mapper->pgr_rom_banks[i]),
              0x4000);  // 16kb
  }

  for (int i = 0; i < mapper->num_chr_banks; i++) {
    file.read(reinterpret_cast<char*>(&mapper->chr_rom_banks[i]),
              0x2000);  // 8kb
  }
}

uint8_t Cartridge::mem_read(uint16_t addr) {
  return mapper->mem_read(addr);
}

void Cartridge::mem_write(uint16_t addr, uint8_t value) {
  mapper->mem_write(addr, value);
}

uint8_t Cartridge::chr_mem_read(uint16_t addr) {
  return mapper->chr_mem_read(addr);
}

void Cartridge::chr_mem_write(uint16_t addr, uint8_t value) {
  mapper->chr_mem_write(addr, value);
}

MirrorMode Cartridge::get_mirror_mode() {
  return mapper->vertical_mirroring ? MirrorMode::VERTICAL
                                    : MirrorMode::HORIZONTAL;
}