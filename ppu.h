#pragma once
#include <cstdint>

struct OAMEntry {
  uint8_t y;
  uint8_t tile;
  uint8_t attributes;
  uint8_t x;
};

class NES;
class PPU {
 public:
  uint8_t pixels[256][256][3];  // y, x, c
  bool frame_ready = false;

  PPU(NES& nes);

  uint8_t mem_read(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);

  uint8_t reg_read(uint16_t addr);
  void reg_write(uint16_t addr, uint8_t value);

  void tick();

  bool rendering_enabled();
  void render_scanline();
  void render_scanline_bg(uint8_t scanline_buffer[256]);
  void render_scanline_sprites(uint8_t scanline_buffer[256]);
  void render_tile(int tile_index, int start_x, int start_y);

 private:
  NES& nes;

  uint8_t CIRAM[0x800];  // 2kb of Console-Internal RAM for 2 nametables
  uint8_t CGRAM[0x1F];   // 32 bytes of Color Generator RAM (only 28 bytes used)
  uint8_t OAM[256];      // 64 entries

  enum class MirrorMode { VERTICAL, HORIZONTAL, SINGLE, FOUR };
  MirrorMode nt_mirror_mode = MirrorMode::SINGLE;

  // registers
  /*
  struct Addr {
    uint8_t coarse_x_scroll : 5;
    uint8_t coarse_y_scroll : 5;
    uint8_t nt_select : 2;
    uint8_t fine_y_scroll : 3;
  };*/

  uint16_t vram_addr;
  uint16_t temp_vram_addr;
  uint8_t fine_x_scroll;
  bool write_toggle = false;

  uint8_t bus_latch;

  struct {
    uint8_t nt_addr : 2;
    uint8_t addr_increment : 1;
    uint8_t sprite_pt_addr : 1;
    uint8_t bg_pt_addr : 1;
    uint8_t sprite_size : 1;
    uint8_t ss : 1;
    uint8_t nmi : 1;
  } PPUCTRL;
  void write_PPUCTRL(uint8_t value);

  struct {
    uint8_t greyscale : 1;
    uint8_t show_bg_left8 : 1;
    uint8_t show_sprites_left8 : 1;
    uint8_t show_bg : 1;
    uint8_t show_sprites : 1;
    uint8_t tint_red : 1;
    uint8_t tint_green : 1;
    uint8_t tint_blue : 1;
  } PPUMASK;
  void write_PPUMASK(uint8_t value);

  struct {
    uint8_t sprite_overflow : 1;
    uint8_t sprite_0_hit : 1;
    uint8_t in_vblank : 1;
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

  uint16_t nt_mirror_addr(uint16_t addr);
};