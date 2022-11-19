#include "ppu.h"
#include <cstring>
#include <stdexcept>
#include "nes.h"

PPU::PPU(NES& nes) : nes(nes) {}

void PPU::power_on() {
  scanline = 0;
  scanline_cycle = 0;
  odd_frame = false;
  frame_ready = false;

  PPUCTRL.raw = 0x00;
  PPUMASK.raw = 0x00;
  PPUSTATUS.raw = 0x00;
  OAMADDR = 0x00;
  write_toggle = false;
  vram_addr.raw = 0x0000;
  temp_vram_addr.raw = 0x0000;
  fine_x_scroll = 0;
  bus_latch = 0x00;

  memset(CIRAM, 0x00, 0x800);
  memset(CGRAM, 0x00, 32);
  memset(OAM, 0x00, 256);
  memset(pixels, 0x00, 240 * 256 * 3);
}

uint16_t PPU::nt_mirror_addr(uint16_t addr) {
  switch (nes.cartridge.get_mirror_mode()) {
    case MirrorMode::HORIZONTAL:
      return ((addr >> 1) & 0x400) | (addr & 0x3FF);
    case MirrorMode::VERTICAL:
      return addr & 0x7FF;
    case MirrorMode::SINGLE_LOWER:
      return addr & 0x3FF;
    case MirrorMode::SINGLE_UPPER:
      return 0x400 | (addr & 0x3FF);
    case MirrorMode::FOUR:
    default:
      throw std::runtime_error("unimplemented");
  }
}

namespace {

const uint32_t rgb_palette[64] = {
    0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000,
    0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000,
    0x000000, 0x000000, 0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC,
    0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844,
    0x008888, 0x000000, 0x000000, 0x000000, 0xF8F8F8, 0x3CBCFC, 0x6888FC,
    0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818,
    0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000, 0xFCFCFC,
    0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
    0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000,
    0x000000,
};

uint16_t palette_addr(uint16_t addr) {
  addr = addr & 0x001F;
  if ((addr & 0x13) == 0x10) {
    addr = addr & 0x0F;
  }
  return addr;
}
}  // namespace

uint8_t PPU::mem_read(uint16_t addr) {
  addr &= 0x3FFF;
  if (addr <= 0x1FFF) {
    // Pattern tables
    return nes.cartridge.chr_mem_read(addr);
  } else if (addr <= 0x3EFF) {
    // Nametables
    return CIRAM[nt_mirror_addr(addr)];
  } else if (addr <= 0x3FFF) {
    // Palettes
    return CGRAM[palette_addr(addr)];
  }
  return 0;
}

void PPU::mem_write(uint16_t addr, uint8_t value) {
  addr &= 0x3FFF;
  if (addr <= 0x1FFF) {
    // Pattern tables
    nes.cartridge.chr_mem_write(addr, value);
  } else if (addr <= 0x3EFF) {
    // Nametables
    CIRAM[nt_mirror_addr(addr)] = value;
  } else if (addr <= 0x3FFF) {
    // Palettes
    CGRAM[palette_addr(addr)] = value;
  }
}

uint8_t PPU::port_read(uint16_t addr) {
  switch (addr) {
    case 0x2002:
      return read_PPUSTATUS();
    case 0x2004:
      return OAM[OAMADDR];
    case 0x2007:
      return read_PPUDATA();
  }
  return bus_latch;
}

void PPU::port_write(uint16_t addr, uint8_t value) {
  bus_latch = value;
  switch (addr) {
    case 0x2000:
      return write_PPUCTRL(value);
    case 0x2001:
      return write_PPUMASK(value);
    case 0x2003:
      OAMADDR = value;
      break;
    case 0x2004:
      OAM[OAMADDR++] = value;
      break;
    case 0x2005:
      return write_PPUSCROLL(value);
    case 0x2006:
      return write_PPUADDR(value);
    case 0x2007:
      return write_PPUDATA(value);
  }
}

void PPU::write_PPUCTRL(uint8_t value) {
  uint8_t old_nmi = PPUCTRL.nmi;
  PPUCTRL.raw = value;
  if (PPUSTATUS.in_vblank == 1 && PPUCTRL.nmi == 1 && old_nmi == 0) {
    nes.cpu.request_NMI();
  }
  temp_vram_addr.nt_select = PPUCTRL.nt_addr;
}

void PPU::write_PPUMASK(uint8_t value) {
  PPUMASK.raw = value;
}

uint8_t PPU::read_PPUSTATUS() {
  write_toggle = false;
  uint8_t value = (bus_latch & 0x1F) | (PPUSTATUS.raw & 0xE0);
  PPUSTATUS.in_vblank = 0;
  return value;
}

void PPU::write_PPUSCROLL(uint8_t value) {
  if (!write_toggle) {
    temp_vram_addr.coarse_x_scroll = value >> 3;
    fine_x_scroll = value & 0x07;
  } else {
    temp_vram_addr.coarse_y_scroll = value >> 3;
    temp_vram_addr.fine_y_scroll = value & 0x07;
  }
  write_toggle = !write_toggle;
}

void PPU::write_PPUADDR(uint8_t value) {
  if (!write_toggle) {
    temp_vram_addr.raw = (temp_vram_addr.raw & 0x00FF) | ((value & 0x3F) << 8);
  } else {
    temp_vram_addr.raw = (temp_vram_addr.raw & 0xFF00) | value;
    vram_addr.raw = temp_vram_addr.raw;
  }
  write_toggle = !write_toggle;
}

uint8_t PPU::read_PPUDATA() {
  uint8_t value;
  if (vram_addr.raw <= 0x3EFF) {
    value = data_buffer;
    data_buffer = mem_read(vram_addr.raw);
  } else {
    value = mem_read(vram_addr.raw);
    data_buffer = mem_read(vram_addr.raw - 0x1000);
  }
  vram_addr.raw += PPUCTRL.addr_increment ? 32 : 1;
  return value;
}

void PPU::write_PPUDATA(uint8_t value) {
  mem_write(vram_addr.raw, value);
  vram_addr.raw += PPUCTRL.addr_increment ? 32 : 1;
}

void PPU::tick() {
  if (scanline <= 239) {
    // Visible line
    render_scanline();
  } else if (scanline == 241 && scanline_cycle == 1) {
    PPUSTATUS.in_vblank = 1;
    if (PPUCTRL.nmi == 1) {
      nes.cpu.request_NMI();
    }
  } else if (scanline == 261) {
    render_scanline();
    if (scanline_cycle == 1) {
      PPUSTATUS.in_vblank = 0;
      PPUSTATUS.sprite_0_hit = 0;
      PPUSTATUS.sprite_overflow = 0;
    } else if (scanline_cycle >= 280 && scanline_cycle <= 304) {
      if (rendering_enabled()) {
        vram_addr.coarse_y_scroll = temp_vram_addr.coarse_y_scroll;
        vram_addr.fine_y_scroll = temp_vram_addr.fine_y_scroll;
        vram_addr.nt_select =
            (vram_addr.nt_select & 0x1) | (temp_vram_addr.nt_select & 0x2);
      }
    } else if (scanline_cycle == 340) {
      if (odd_frame && rendering_enabled()) {
        // Skip next idle cycle
        scanline_cycle++;
      }
    }
  }
  scanline_cycle++;
  if (scanline_cycle >= 341) {
    scanline_cycle = scanline_cycle % 341;  // modulo for the odd frame case
    scanline++;
    if (scanline > 261) {
      scanline = 0;
      frame_ready = true;
      odd_frame = !odd_frame;
    }
  }
}

bool PPU::rendering_enabled() {
  return PPUMASK.show_bg || PPUMASK.show_sprites;
}

void PPU::update_shift_registers() {
  pt_shift_register[0] <<= 1;
  pt_shift_register[1] <<= 1;
  at_shift_register[0] = (at_shift_register[0] << 1) | at_latch[0];
  at_shift_register[1] = (at_shift_register[1] << 1) | at_latch[1];
}

void PPU::reload_shift_registers() {
  pt_shift_register[0] = (pt_shift_register[0] & 0xFF00) | pt_byte[0];
  pt_shift_register[1] = (pt_shift_register[1] & 0xFF00) | pt_byte[1];
  at_latch[0] = at_byte & 0x1;
  at_latch[1] = (at_byte & 0x2) >> 1;
}

void PPU::render_scanline() {
  if (!rendering_enabled() || scanline_cycle == 0) {
    return;
  }

  // Sprite pipeline
  // TODO: Make cycle accurate?
  if (scanline != 261) {
    if (scanline_cycle == 1) {
      clear_secondary_oam();
    } else if (scanline_cycle == 64) {
      evaluate_sprites();
    }
  }
  if (scanline_cycle == 257) {
    load_rendering_oam();
  }

  if (scanline_cycle <= 256 ||
      (scanline_cycle >= 321 && scanline_cycle <= 336)) {
    uint16_t nt_addr, at_addr, pt_addr_base;
    int at_shift;
    switch (scanline_cycle % 8) {
      case 1:
        // Fetch name table byte and reload shift registers/latches
        nt_addr = 0x2000 | (vram_addr.raw & 0x0FFF);
        if (scanline_cycle != 1 && scanline_cycle != 321) {
          reload_shift_registers();
        }
        nt_byte = mem_read(nt_addr);
        break;
      case 3:
        // Fetch attribute table byte
        at_addr = 0x23C0 | (vram_addr.nt_select << 10) |
                  ((vram_addr.coarse_y_scroll >> 2) << 3) |
                  (vram_addr.coarse_x_scroll >> 2);
        at_shift = (vram_addr.coarse_x_scroll & 0x0002) |
                   ((vram_addr.coarse_y_scroll & 0x0002) << 1);
        at_byte = mem_read(at_addr) >> at_shift;
        break;
      case 5:
        // Fetch pattern table lo byte
        pt_addr_base = (PPUCTRL.bg_pt_addr << 12) | (nt_byte * 16);
        pt_byte[0] = mem_read(pt_addr_base | vram_addr.fine_y_scroll);
        break;
      case 7:
        // Fetch pattern table hi byte
        pt_addr_base = (PPUCTRL.bg_pt_addr << 12) | (nt_byte * 16);
        pt_byte[1] = mem_read(pt_addr_base | 0x0008 | vram_addr.fine_y_scroll);
        break;
      case 0:
        // Increment x
        if (vram_addr.coarse_x_scroll == 31) {
          vram_addr.coarse_x_scroll = 0;
          vram_addr.nt_select = vram_addr.nt_select ^ 0x1;
        } else {
          vram_addr.coarse_x_scroll++;
        }
        break;
    }
    if (scanline_cycle == 256) {
      // Increment y
      if (vram_addr.fine_y_scroll != 0x0007) {
        vram_addr.fine_y_scroll++;
      } else {
        vram_addr.fine_y_scroll = 0;
        if (vram_addr.coarse_y_scroll == 29) {
          vram_addr.coarse_y_scroll = 0;
          vram_addr.nt_select = vram_addr.nt_select ^ 0x2;
        } else if (vram_addr.coarse_y_scroll == 31) {
          vram_addr.coarse_y_scroll = 0;
        } else {
          vram_addr.coarse_y_scroll++;
        }
      }
    }

    // Render pixel
    if (scanline != 261 && scanline_cycle <= 256) {
      render_pixel();
    }
    update_shift_registers();
  } else if (scanline_cycle == 257) {
    reload_shift_registers();

    // Reset x
    vram_addr.coarse_x_scroll = temp_vram_addr.coarse_x_scroll;
    vram_addr.nt_select =
        (vram_addr.nt_select & 0x2) | (temp_vram_addr.nt_select & 0x1);
  } else if (scanline_cycle == 337) {
    reload_shift_registers();
  }
}

void PPU::clear_secondary_oam() {
  memset(secondary_oam, 0xFF, 8 * sizeof(OAMEntry));
}

void PPU::evaluate_sprites() {
  int sprite_height = PPUCTRL.sprite_size ? 16 : 8;
  int n = 0;
  for (int i = 0; i < 64; i++) {
    uint8_t y = OAM[i * 4 + 0];
    if (scanline >= y && scanline < y + sprite_height) {
      if (n == 8) {
        PPUSTATUS.sprite_overflow = 1;
        break;
      }
      OAMEntry& entry = secondary_oam[n++];
      entry.id = i;
      entry.y = y;
      entry.tile = OAM[i * 4 + 1];
      entry.attributes = OAM[i * 4 + 2];
      entry.x = OAM[i * 4 + 3];
    }
  }
}

void PPU::load_rendering_oam() {
  int sprite_height = PPUCTRL.sprite_size ? 16 : 8;
  for (int i = 0; i < 8; i++) {
    rendering_oam[i] = secondary_oam[i];
    OAMEntry& entry = rendering_oam[i];
    if (entry.id == 0xFF) {
      continue;
    }
    uint8_t flip_y = (entry.attributes >> 7) & 0x01;
    int y = (scanline - entry.y) % sprite_height;
    if (flip_y) {
      y = sprite_height - 1 - y;
    }
    uint8_t tile_index = entry.tile;
    uint16_t pt_addr_base;
    if (sprite_height == 16) {
      pt_addr_base = ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4);
      y = y + (y & 0x08);
    } else {
      pt_addr_base = (PPUCTRL.sprite_pt_addr << 12) | (tile_index << 4);
    }
    entry.data_lo = mem_read(pt_addr_base | y);
    entry.data_hi = mem_read(pt_addr_base | 0x0008 | y);
  }
}

void PPU::render_pixel() {
  int x = scanline_cycle - 1;
  uint8_t palette = 0;

  // Background pixel
  if (PPUMASK.show_bg && (PPUMASK.show_bg_left8 || x > 8)) {
    int pt_shift = 15 - fine_x_scroll;
    uint8_t color_index = ((pt_shift_register[1] >> pt_shift) & 0x1) << 1;
    color_index |= ((pt_shift_register[0] >> pt_shift) & 0x1);
    int at_shift = 7 - fine_x_scroll;
    uint8_t palette_index = (((at_shift_register[1] >> at_shift) & 0x1) << 1) |
                            ((at_shift_register[0] >> at_shift) & 0x1);
    palette = color_index;
    if (color_index != 0) {
      palette |= (palette_index << 2);
    }
  }

  // Sprite pixel
  if (PPUMASK.show_sprites && (PPUMASK.show_sprites_left8 || x > 8)) {
    uint8_t sprite_palette = 0;
    for (int i = 0; i < 8; i++) {
      OAMEntry& entry = rendering_oam[i];
      if (entry.id == 0xFF) {
        break;
      }
      uint8_t flip_x = (entry.attributes >> 6) & 0x01;
      uint8_t priority = (entry.attributes >> 5) & 0x01;
      int sprite_x = x - entry.x;
      if (sprite_x < 0 || sprite_x >= 8) {
        continue;
      }
      uint8_t palette_index = entry.attributes & 0x03;
      int shift_x = flip_x ? sprite_x : 7 - sprite_x;
      uint8_t color_index = ((entry.data_hi >> shift_x) & 0x1) << 1;
      color_index |= ((entry.data_lo >> shift_x) & 0x1);
      if (color_index == 0) {
        continue;
      }
      uint8_t sprite_palette = 0x10 | (palette_index << 2) | color_index;
      if (palette == 0) {
        palette = sprite_palette;
      } else {
        if (entry.id == 0) {
          // TODO: Technically cycle 2 is the earliest this can be set
          PPUSTATUS.sprite_0_hit = 1;
        }
        if (priority == 0) {
          palette = sprite_palette;
        }
      }
      break;
    }
  }

  uint8_t color = mem_read(0x3F00 | palette);
  uint32_t rgb = rgb_palette[color];
  pixels[scanline][x][0] = (rgb >> 16) & 0xFF;
  pixels[scanline][x][1] = (rgb >> 8) & 0xFF;
  pixels[scanline][x][2] = (rgb >> 0) & 0xFF;
}

void PPU::render_nametables(uint8_t (&out)[480][512][3]) {
  for (int i = 0; i < 4; i++) {
    uint16_t base_addr = 0x2000 + i * 0x0400;
    int start_x = (i % 2) * 256;
    int start_y = (i / 2) * 240;
    for (int y = 0; y < 30; y++) {
      for (int x = 0; x < 32; x++) {
        uint8_t tile_index = nes.ppu.mem_read(base_addr + y * 32 + x);
        uint16_t pt_addr = (PPUCTRL.bg_pt_addr << 12) | (tile_index << 4);
        uint16_t at_addr = base_addr | 0x03C0 | ((y >> 2) << 3) | (x >> 2);
        int at_shift = (x & 0x0002) | ((y & 0x0002) << 1);
        uint8_t palette_index = (mem_read(at_addr) >> at_shift) & 0x03;
        render_tile(pt_addr, palette_index, start_x + x * 8, start_y + y * 8,
                    512, &out[0][0][0]);
      }
    }
  }
}

void PPU::render_pattern_tables(uint8_t (&out)[128][256][3]) {
  for (int i = 0; i < 2; i++) {
    int start_x = i * 128;
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        uint8_t tile_index = i * 256 + y * 16 + x;
        uint16_t pt_addr = i * 0x1000 + (tile_index << 4);
        render_tile(pt_addr, 0, start_x + x * 8, y * 8, 256, &out[0][0][0],
                    true);
      }
    }
  }
}

void PPU::render_tile(uint16_t tile_addr,
                      uint8_t palette_index,
                      int start_x,
                      int start_y,
                      int stride_x,
                      uint8_t* out,
                      bool greyscale) {
  uint16_t pt_addr_base = tile_addr;
  for (int y = 0; y < 8; y++) {
    uint8_t color_lo = mem_read(pt_addr_base | y);
    uint8_t color_hi = mem_read(pt_addr_base | 0x0008 | y);

    for (int x = 0; x < 8; x++) {
      uint8_t color_index = ((color_hi >> (7 - x)) & 0x01) << 1;
      color_index |= ((color_lo >> (7 - x)) & 0x01);

      uint8_t palette = color_index;
      if (color_index != 0) {
        palette |= (palette_index << 2);
      }
      uint32_t rgb = 0;
      if (greyscale) {
        uint8_t c = color_index * 85;
        rgb = c | (c << 8) | (c << 16);
      } else {
        uint8_t color = mem_read(0x3F00 | palette);
        rgb = rgb_palette[color];
      }
      int i = (start_y + y) * stride_x + start_x + x;
      out[i * 3 + 0] = (rgb >> 16) & 0xFF;
      out[i * 3 + 1] = (rgb >> 8) & 0xFF;
      out[i * 3 + 2] = (rgb >> 0) & 0xFF;
    }
  }
}