#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ppu.h"

class NES;

struct ROMData {
  char header[16];
  std::vector<uint8_t> pgr_rom;
  std::vector<uint8_t> chr_rom;
};

class Mapper {
 public:
  NES* nes = nullptr;
  std::vector<uint8_t> pgr_rom;
  std::vector<uint8_t> chr_rom;
  uint8_t pgr_ram[0x2000];  // 8kb
  int pgr_map[4];           // 8kb (0x2000) blocks
  int chr_map[8];           // 1kb (0x400) blocks

  int num_ram_banks = 0;
  bool has_trainer = false;
  bool has_ram = false;
  MirrorMode mirror_mode = MirrorMode::VERTICAL;

  Mapper() = default;
  Mapper(ROMData& rom_data);
  virtual ~Mapper() = default;

  virtual uint8_t mem_read(uint16_t addr);
  virtual void mem_write(uint16_t addr, uint8_t value);
  virtual uint8_t chr_mem_read(uint16_t addr);
  virtual void chr_mem_write(uint16_t addr, uint8_t value);

  void set_pgr_map(uint16_t bank_size, uint8_t from_bank, uint8_t to_bank);
  void set_chr_map(uint16_t bank_size, uint8_t from_bank, uint8_t to_bank);
  void set_nes(NES* nes);
  virtual void signal_scanline() {}
};

class Cartridge {
 public:
  NES& nes;
  std::unique_ptr<Mapper> mapper;

  Cartridge(NES& nes) : nes(nes) {}
  bool load(const char* filename);

  uint8_t mem_read(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);
  uint8_t chr_mem_read(uint16_t addr);
  void chr_mem_write(uint16_t addr, uint8_t value);

  MirrorMode get_mirror_mode();
  void signal_scanline();
};
