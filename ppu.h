#pragma once
#include <cstdint>
#include "bitfield.h"

struct OAMEntry {
  uint8_t id;
  uint8_t y;
  uint8_t tile;
  uint8_t attributes;
  uint8_t x;
  uint8_t data_hi;
  uint8_t data_lo;
};

enum class MirrorMode { VERTICAL, HORIZONTAL, SINGLE, FOUR };

class NES;
class PPU {
 public:
  uint8_t pixels[256][256][3];  // y, x, c
  bool frame_ready = false;

  PPU(NES& nes);

  uint8_t mem_read(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);
  uint8_t port_read(uint16_t addr);
  void port_write(uint16_t addr, uint8_t value);

  void tick();
  bool rendering_enabled();
  void render_pixel();
  void render_scanline();
  void render_tile(int tile_index, int start_x, int start_y);

 private:
  NES& nes;

  uint8_t CIRAM[0x800];  // 2kb of Console-Internal RAM for 2 nametables
  uint8_t CGRAM[32];     // 32 bytes of Color Generator RAM (only 28 bytes used)
  uint8_t OAM[256];      // 64 entries

  MirrorMode nt_mirror_mode = MirrorMode::VERTICAL;
  uint16_t nt_mirror_addr(uint16_t addr);

  // registers
  union Addr {
    uint16_t raw;
    BitField16<0, 5> coarse_x_scroll;
    BitField16<5, 5> coarse_y_scroll;
    BitField16<10, 2> nt_select;
    BitField16<12, 3> fine_y_scroll;
  };

  Addr vram_addr;
  Addr temp_vram_addr;
  uint8_t fine_x_scroll;
  bool write_toggle = false;
  uint8_t bus_latch;

  union {
    uint8_t raw;
    BitField8<0, 2> nt_addr;
    BitField8<2, 1> addr_increment;
    BitField8<3, 1> sprite_pt_addr;
    BitField8<4, 1> bg_pt_addr;
    BitField8<5, 1> sprite_size;
    BitField8<6, 1> ss;
    BitField8<7, 1> nmi;
  } PPUCTRL;
  void write_PPUCTRL(uint8_t value);

  union {
    uint8_t raw;
    BitField8<0, 1> greyscale;
    BitField8<1, 1> show_bg_left8;
    BitField8<2, 1> show_sprites_left8;
    BitField8<3, 1> show_bg;
    BitField8<4, 1> show_sprites;
    BitField8<5, 1> tint_red;
    BitField8<6, 1> tint_green;
    BitField8<7, 1> tint_blue;
  } PPUMASK;
  void write_PPUMASK(uint8_t value);

  union {
    uint8_t raw;
    BitField8<5, 1> sprite_overflow;
    BitField8<6, 1> sprite_0_hit;
    BitField8<7, 1> in_vblank;
  } PPUSTATUS;
  uint8_t read_PPUSTATUS();

  uint8_t OAMADDR;

  void write_PPUSCROLL(uint8_t value);
  void write_PPUADDR(uint8_t value);

  uint8_t data_buffer;
  uint8_t read_PPUDATA();
  void write_PPUDATA(uint8_t value);

  // state
  int scanline = 0;
  int scanline_cycle = 0;
  bool odd_frame = false;

  // For pairs, 0-index is the lo bit/byte
  uint8_t at_shift_register[2];
  uint8_t at_latch[2];
  uint16_t pt_shift_register[2];
  uint8_t nt_byte;
  uint8_t at_byte;
  uint8_t pt_byte[2];
  void update_shift_registers();
  void reload_shift_registers();

  OAMEntry secondary_oam[8];
  OAMEntry rendering_oam[8];

  void clear_secondary_oam();
  void evaluate_sprites();
  void load_rendering_oam();
};