#include "cartridge.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include "nes.h"

Mapper::Mapper(ROMData& rom_data)
    : pgr_rom(std::move(rom_data.pgr_rom)),
      chr_rom(std::move(rom_data.chr_rom)) {
  uint8_t rom_ctrl1 = rom_data.header[6];
  uint8_t rom_ctrl2 = rom_data.header[7];
  num_ram_banks = std::max(1, (int)rom_data.header[8]);
  has_trainer = rom_ctrl1 & 0x04;
  has_ram = rom_ctrl1 & 0x02;
  mirror_mode =
      rom_ctrl1 & 0x01 ? MirrorMode::VERTICAL : MirrorMode::HORIZONTAL;
}

uint8_t Mapper::mem_read(uint16_t addr) {
  if (addr < 0x6000) {
    throw std::runtime_error("Expansion ROM not supported");
  } else if (addr < 0x8000) {
    return pgr_ram[addr - 0x6000];
  } else {
    addr -= 0x8000;
    return pgr_rom[pgr_map[addr / 0x2000] + addr % 0x2000];
  }
}

void Mapper::mem_write(uint16_t addr, uint8_t value) {
  if (addr >= 0x6000 && addr < 0x8000) {
    pgr_ram[addr - 0x6000] = value;
  }
}

uint8_t Mapper::chr_mem_read(uint16_t addr) {
  addr &= 0x1FFF;
  return chr_rom[chr_map[addr / 0x400] + addr % 0x400];
}

void Mapper::chr_mem_write(uint16_t addr, uint8_t value) {
  addr &= 0x1FFF;
  chr_rom[chr_map[addr / 0x400] + addr % 0x400] = value;
}

void Mapper::set_pgr_map(uint16_t bank_size,
                         uint8_t from_bank,
                         uint8_t to_bank) {
  int num_subbanks = bank_size / 0x2000;
  for (int i = 0; i < num_subbanks; i++) {
    int index = from_bank * num_subbanks + i;
    int addr = (to_bank * num_subbanks + i) * 0x2000;
    if (index < 4 && addr < pgr_rom.size()) {
      pgr_map[index] = addr;
    }
  }
}

void Mapper::set_chr_map(uint16_t bank_size,
                         uint8_t from_bank,
                         uint8_t to_bank) {
  int num_subbanks = bank_size / 0x400;
  for (int i = 0; i < num_subbanks; i++) {
    int index = from_bank * num_subbanks + i;
    int addr = (to_bank * num_subbanks + i) * 0x400;
    if (index < 8 && addr < chr_rom.size()) {
      chr_map[index] = addr;
    }
  }
}

void Mapper::set_nes(NES* nes) {
  this->nes = nes;
}

// Dummy mapper for load failures
class MapperDummy : public Mapper {
 public:
  MapperDummy() {}
  uint8_t mem_read(uint16_t addr) { return 0; }
  void mem_write(uint16_t addr, uint8_t value) {}
  uint8_t chr_mem_read(uint16_t addr) { return 0; }
  void chr_mem_write(uint16_t addr, uint8_t value) {}
};

class Mapper0 : public Mapper {
 public:
  Mapper0(ROMData& rom_data) : Mapper(rom_data) {
    if (pgr_rom.size() == 0x8000) {  // 32kb
      set_pgr_map(0x8000, 0, 0);
    } else {  // 16kb
      set_pgr_map(0x4000, 0, 0);
      set_pgr_map(0x4000, 1, 0);
    }
    set_chr_map(0x2000, 0, 0);
  }
};

class Mapper1 : public Mapper {
 public:
  uint8_t shift_register = 0x10;
  uint8_t control = 0;

  Mapper1(ROMData& rom_data) : Mapper(rom_data) {
    set_pgr_map(0x4000, 0, 0);
    set_pgr_map(0x4000, 1, pgr_rom.size() / 0x4000 - 1);
    set_chr_map(0x2000, 0, 0);
  }

  void set_control(uint8_t value) {
    control = value;
    switch (control & 0x03) {
      case 0:
        mirror_mode = MirrorMode::SINGLE_LOWER;
        break;
      case 1:
        mirror_mode = MirrorMode::SINGLE_UPPER;
        break;
      case 2:
        mirror_mode = MirrorMode::VERTICAL;
        break;
      case 3:
        mirror_mode = MirrorMode::HORIZONTAL;
        break;
    }
  }

  void set_chr_bank0(uint8_t value) {
    if (control & 0x10) {
      // Switch two separate 4 KB banks
      set_chr_map(0x1000, 0, value);
    } else {
      // Switch 8 KB at a time
      set_chr_map(0x2000, 0, value >> 1);
    }
  }

  void set_chr_bank1(uint8_t value) {
    if (control & 0x10) {
      // Switch two separate 4 KB banks
      set_chr_map(0x1000, 1, value);
    }
  }

  void set_pgr_bank(uint8_t value) {
    uint8_t pgr_bank_mode = (control >> 2) & 0x03;
    if (pgr_bank_mode == 0 || pgr_bank_mode == 1) {
      // Switch 32 KB at $8000, ignoring low bit of bank number
      int bank = (value & 0x0E) >> 1;
      set_pgr_map(0x8000, 0, bank);
    } else if (pgr_bank_mode == 2) {
      // Fix first bank at $8000 and switch 16 KB bank at $C000
      set_pgr_map(0x4000, 0, 0);
      set_pgr_map(0x4000, 1, value & 0x0F);
    } else if (pgr_bank_mode == 3) {
      // Fix last bank at $C000 and switch 16 KB bank at $8000
      set_pgr_map(0x4000, 0, value & 0x0F);
      set_pgr_map(0x4000, 1, pgr_rom.size() / 0x4000 - 1);
    }
  }

  void mem_write(uint16_t addr, uint8_t value) override {
    Mapper::mem_write(addr, value);
    if (addr >= 0x8000) {
      if (value & 0x80) {
        shift_register = 0x10;
        control |= 0x0C;
      } else {
        bool full = shift_register & 0x01;
        shift_register >>= 1;
        shift_register |= ((value & 0x01) << 4);
        if (full) {
          int index = (addr >> 13) & 0x0003;  // 13th and 14th bits
          switch (index) {
            case 0:  // Control
              set_control(shift_register);
              break;
            case 1:
              set_chr_bank0(shift_register);
              break;
            case 2:
              set_chr_bank1(shift_register);
              break;
            case 3:
              set_pgr_bank(shift_register);
              break;
          }
          shift_register = 0x10;
        }
      }
    }
  }
};

class Mapper2 : public Mapper {
 public:
  Mapper2(ROMData& rom_data) : Mapper(rom_data) {
    set_pgr_map(0x4000, 0, 0);
    set_pgr_map(0x4000, 1, pgr_rom.size() / 0x4000 - 1);
    set_chr_map(0x2000, 0, 0);
  }

  void mem_write(uint16_t addr, uint8_t value) override {
    Mapper::mem_write(addr, value);
    if (addr >= 0x8000) {
      // Select 16 KB PRG ROM bank for CPU $8000-$BFFF
      set_pgr_map(0x4000, 0, value & 0x0F);
    }
  }
};

class Mapper3 : public Mapper0 {
 public:
  Mapper3(ROMData& rom_data) : Mapper0(rom_data) {}

  void mem_write(uint16_t addr, uint8_t value) override {
    Mapper::mem_write(addr, value);
    if (addr >= 0x8000) {
      // Select 8 KB CHR ROM bank for PPU $0000-$1FFF
      set_chr_map(0x2000, 0, value & 0x03);
    }
  }
};

class Mapper4 : public Mapper0 {
 public:
  uint8_t bank_select = 0;
  uint8_t bank_registers[8] = {0};
  uint8_t irq_period = 0;
  bool irq_enabled = false;
  uint8_t irq_counter = 0;

  Mapper4(ROMData& rom_data) : Mapper0(rom_data) {
    set_pgr_map(0x2000, 3, pgr_rom.size() / 0x2000 - 1);
    set_banks();
  }

  void mem_write(uint16_t addr, uint8_t value) override {
    if (addr < 0x8000) {
      Mapper::mem_write(addr, value);
      return;
    }
    switch (addr & 0xE001) {
      case 0x8000:
        bank_select = value;
        set_banks();
        break;
      case 0x8001:
        bank_registers[bank_select & 0x07] = value;
        set_banks();
        break;
      case 0xA000:
        mirror_mode =
            value & 0x01 ? MirrorMode::HORIZONTAL : MirrorMode::VERTICAL;
        break;
      case 0xA001:
        // RAM protect, not implemented
        break;
      case 0xC000:
        irq_period = value;
        break;
      case 0xC001:
        irq_counter = 0;
        break;
      case 0xE000:
        irq_enabled = false;
        nes->cpu.set_irq(IRQType::MMC3, false);
        break;
      case 0xE001:
        irq_enabled = true;
        break;
    }
  }

  void signal_scanline() override {
    if (irq_counter == 0) {
      irq_counter = irq_period;
    } else {
      irq_counter--;
      if (irq_counter == 0 && irq_enabled) {
        nes->cpu.set_irq(IRQType::MMC3, true);
      }
    }
  }

  void set_banks() {
    uint8_t chr_mode = (bank_select >> 7) & 0x01;
    uint8_t pgr_mode = (bank_select >> 6) & 0x01;
    // CHR banks
    if (chr_mode == 0) {
      set_chr_map(0x800, 0, bank_registers[0] >> 1);
      set_chr_map(0x800, 1, bank_registers[1] >> 1);
      for (int i = 0; i < 4; i++) {
        set_chr_map(0x400, 4 + i, bank_registers[i + 2]);
      }
    } else {
      for (int i = 0; i < 4; i++) {
        set_chr_map(0x400, i, bank_registers[i + 2]);
      }
      set_chr_map(0x800, 2, bank_registers[0] >> 1);
      set_chr_map(0x800, 3, bank_registers[1] >> 1);
    }
    // PGR banks
    if (pgr_mode == 0) {
      set_pgr_map(0x2000, 0, bank_registers[6] & 0x3F);
      set_pgr_map(0x2000, 1, bank_registers[7] & 0x3F);
      set_pgr_map(0x2000, 2, pgr_rom.size() / 0x2000 - 2);
    } else {
      set_pgr_map(0x2000, 0, pgr_rom.size() / 0x2000 - 2);
      set_pgr_map(0x2000, 1, bank_registers[7] & 0x3F);
      set_pgr_map(0x2000, 2, bank_registers[6] & 0x3F);
    }
  }
};

namespace {
std::unique_ptr<Mapper> get_mapper(int mapper_num, ROMData& rom_data) {
  switch (mapper_num) {
    case 0:
      return std::make_unique<Mapper0>(rom_data);
    case 1:
      return std::make_unique<Mapper1>(rom_data);
    case 2:
      return std::make_unique<Mapper2>(rom_data);
    case 3:
      return std::make_unique<Mapper3>(rom_data);
    case 4:
      return std::make_unique<Mapper4>(rom_data);
    default:
      return nullptr;
  }
}
}  // namespace

bool Cartridge::load(const char* filename) {
  printf("Loading %s...\n", filename);
  mapper = std::make_unique<MapperDummy>();
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    fprintf(stderr, "Could not load cartridge file %s\n", filename);
    return false;
  }

  ROMData rom_data;
  file.read(rom_data.header, 16);
  if (!(rom_data.header[0] == 'N' && rom_data.header[1] == 'E' &&
        rom_data.header[2] == 'S' && rom_data.header[3] == 0x1A)) {
    fprintf(stderr, "Invalid cartridge header\n");
    return false;
  }

  int num_pgr_banks = rom_data.header[4];
  int num_chr_banks = rom_data.header[5];
  uint8_t rom_ctrl1 = rom_data.header[6];
  uint8_t rom_ctrl2 = rom_data.header[7];
  int mapper_num = (rom_ctrl2 & 0xF0) | (rom_ctrl1 >> 4);
  printf("Mapper: %d\n", mapper_num);
  printf("PGR banks: %d, CHR banks: %d\n", num_pgr_banks, num_chr_banks);

  rom_data.pgr_rom.resize(num_pgr_banks * 0x4000);
  for (int i = 0; i < num_pgr_banks; i++) {
    file.read(reinterpret_cast<char*>(rom_data.pgr_rom.data() + i * 0x4000),
              0x4000);  // 16kb
  }
  rom_data.chr_rom.resize(std::max(num_chr_banks * 0x2000, 0x2000));
  for (int i = 0; i < num_chr_banks; i++) {
    file.read(reinterpret_cast<char*>(rom_data.chr_rom.data() + i * 0x2000),
              0x2000);  // 8kb
  }

  mapper = get_mapper(mapper_num, rom_data);
  if (mapper == nullptr) {
    fprintf(stderr, "Unsupported mapper type %d\n", mapper_num);
    return false;
  }
  mapper->set_nes(&nes);

  if (mapper->has_trainer) {
    // TODO
    fprintf(stderr, "Trainer unhandled\n");
    return false;
  }
  return true;
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
  return mapper->mirror_mode;
}

void Cartridge::signal_scanline() {
  mapper->signal_scanline();
}