#pragma once
#include <cstdint>
#include <memory>
#include "ppu.h"

class Mapper {
 public:
  uint8_t pgr_rom_banks[8][0x4000];  // 16kb each
  uint8_t chr_rom_banks[8][0x2000];  // 8kb each

  int num_pgr_banks;
  int num_chr_banks;
  int num_ram_banks;
  bool has_trainer;
  bool has_ram;
  bool vertical_mirroring;

  // Mapper() = delete;
  Mapper(char header[]);
  virtual ~Mapper() = default;

  virtual uint8_t mem_read(uint16_t addr) = 0;
  virtual void mem_write(uint16_t addr, uint8_t value) = 0;
  virtual uint8_t chr_mem_read(uint16_t addr) = 0;
  virtual void chr_mem_write(uint16_t addr, uint8_t value) = 0;
};

class Cartridge {
 public:
  std::unique_ptr<Mapper> mapper;

  Cartridge() {}

  void load(const char* filename);

  uint8_t mem_read(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);
  uint8_t chr_mem_read(uint16_t addr);
  void chr_mem_write(uint16_t addr, uint8_t value);

  MirrorMode get_mirror_mode();
};
