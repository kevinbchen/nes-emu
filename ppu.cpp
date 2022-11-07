#include "ppu.h"
#include <cstring>
#include <stdexcept>
#include "nes.h"

PPU::PPU(NES& nes) : nes(nes) {}

uint16_t PPU::nt_mirror_addr(uint16_t addr) {
  switch (nt_mirror_mode) {
    case MirrorMode::HORIZONTAL:
      return (addr & 0x3FF) + ((addr >> 1) & 0x400);
    case MirrorMode::VERTICAL:
      return addr & 0x7FF;
    case MirrorMode::SINGLE:
      return addr & 0x3FF;
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
  PPUCTRL.nt_addr = value & 0x3;
  PPUCTRL.addr_increment = (value >> 2) & 0x1;
  PPUCTRL.sprite_pt_addr = (value >> 3) & 0x1;
  PPUCTRL.bg_pt_addr = (value >> 4) & 0x1;
  PPUCTRL.sprite_size = (value >> 5) & 0x1;
  PPUCTRL.ss = (value >> 6) & 0x1;
  PPUCTRL.nmi = (value >> 7) & 0x1;
  if (PPUSTATUS.in_vblank == 1 && PPUCTRL.nmi == 1 && old_nmi == 0) {
    nes.cpu.request_NMI();
  }

  temp_vram_addr = (temp_vram_addr & 0xF3FF) | (PPUCTRL.nt_addr << 10);
}

void PPU::write_PPUMASK(uint8_t value) {
  PPUMASK.greyscale = value & 0x3;
  PPUMASK.show_bg_left8 = (value >> 1) & 0x1;
  PPUMASK.show_sprites_left8 = (value >> 2) & 0x1;
  PPUMASK.show_bg = (value >> 3) & 0x1;
  PPUMASK.show_sprites = (value >> 4) & 0x1;
  PPUMASK.tint_red = (value >> 5) & 0x1;
  PPUMASK.tint_green = (value >> 6) & 0x1;
  PPUMASK.tint_blue = (value >> 7) & 0x1;
}

uint8_t PPU::read_PPUSTATUS() {
  write_toggle = false;
  uint8_t value = (bus_latch & 0x1F) | (PPUSTATUS.sprite_overflow << 5) |
                  (PPUSTATUS.sprite_0_hit << 6) | (PPUSTATUS.in_vblank << 7);
  PPUSTATUS.in_vblank = 0;
  return value;
}

void PPU::write_PPUSCROLL(uint8_t value) {
  if (!write_toggle) {
    temp_vram_addr = (temp_vram_addr & 0xFFE0) | (value >> 3);
    fine_x_scroll = value & 0x07;
  } else {
    temp_vram_addr = (temp_vram_addr & 0x0C1F) | ((value & 0xF8) << 2) |
                     ((value & 0x07) << 12);
  }
  write_toggle = !write_toggle;
}

void PPU::write_PPUADDR(uint8_t value) {
  if (!write_toggle) {
    temp_vram_addr = (temp_vram_addr & 0x00FF) | ((value & 0x3F) << 8);
  } else {
    temp_vram_addr = (temp_vram_addr & 0xFF00) | value;
    vram_addr = temp_vram_addr;
  }
  write_toggle = !write_toggle;
}

uint8_t PPU::read_PPUDATA() {
  uint8_t value;
  if (vram_addr <= 0x3EFF) {
    value = data_buffer;
    data_buffer = mem_read(vram_addr);
  } else {
    value = mem_read(vram_addr);
    data_buffer = mem_read(vram_addr - 0x1000);
  }
  vram_addr += PPUCTRL.addr_increment ? 32 : 1;
  return value;
}

void PPU::write_PPUDATA(uint8_t value) {
  mem_write(vram_addr, value);
  vram_addr += PPUCTRL.addr_increment ? 32 : 1;
}

void PPU::tick() {
  scanline_cycle++;
  if (scanline == 241 && scanline_cycle == 1) {
    PPUSTATUS.in_vblank = 1;
    if (PPUCTRL.nmi == 1) {
      nes.cpu.request_NMI();
    }
  } else if (scanline == 261) {
    if (scanline_cycle == 1) {
      PPUSTATUS.in_vblank = 0;
      PPUSTATUS.sprite_0_hit = 0;
      PPUSTATUS.sprite_overflow = 0;
    } else if (scanline_cycle >= 280 && scanline_cycle <= 304) {
      if (rendering_enabled()) {
        const int mask = 0xFBE0;
        vram_addr = (vram_addr & ~0xFBE0) | (temp_vram_addr & mask);
      }
    }
  }
  if (scanline_cycle >= 341) {
    scanline_cycle = 0;
    if (scanline <= 239) {
      if (rendering_enabled()) {
        render_scanline();
      }
    }
    scanline++;
    if (scanline > 261) {
      scanline = 0;
      frame_ready = true;
    }
  }
}

bool PPU::rendering_enabled() {
  return PPUMASK.show_bg || PPUMASK.show_sprites;
}

void PPU::render_scanline() {
  // TODO: pixel
  uint8_t scanline_buffer[256 + 8];
  memset(scanline_buffer, 0x00, 256);
  if (PPUMASK.show_bg) {
    render_scanline_bg(scanline_buffer);
  }
  if (PPUMASK.show_sprites) {
    render_scanline_sprites(scanline_buffer);
  }
  for (int x = 0; x < 256; x++) {
    uint8_t color = mem_read(0x3F00 | scanline_buffer[x]);
    uint32_t rgb = rgb_palette[color];
    pixels[scanline][x][0] = (rgb >> 16) & 0xFF;
    pixels[scanline][x][1] = (rgb >> 8) & 0xFF;
    pixels[scanline][x][2] = (rgb >> 0) & 0xFF;
  }
}

void PPU::render_scanline_bg(uint8_t scanline_buffer[]) {
  // TODO: fine_x_scroll
  int y = scanline % 8;
  for (int i = 0; i < 32; i++) {
    uint16_t nt_addr = 0x2000 | (vram_addr & 0x0FFF);
    uint8_t tile_index = mem_read(nt_addr);
    uint16_t pt_addr_base = (PPUCTRL.bg_pt_addr << 12) | (tile_index << 4);
    uint8_t color_lo = mem_read(pt_addr_base | y);
    uint8_t color_hi = mem_read(pt_addr_base | 0x0008 | y);

    uint16_t at_addr = 0x23C0 | (vram_addr & 0x0C00) |
                       ((vram_addr >> 4) & 0x38) | ((vram_addr >> 2) & 0x07);

    int at_shift = (vram_addr & 0x0002) | ((vram_addr >> 4) & 0x0004);
    uint8_t palette_index = (mem_read(at_addr) >> at_shift) & 0x03;

    int start_x = i * 8;
    for (int x = 0; x < 8; x++) {
      uint8_t color_index = ((color_hi >> (7 - x)) & 0x1) << 1;
      color_index |= ((color_lo >> (7 - x)) & 0x1);

      uint8_t palette = color_index;
      if (color_index != 0) {
        palette |= (palette_index << 2);
      }
      scanline_buffer[start_x + x] = palette;
    }

    // Increment x
    vram_addr = (vram_addr & 0xFFE0) | ((vram_addr + 1) & 0x001F);
  }

  // Increment y
  if ((vram_addr & 0x7000) != 0x7000) {
    vram_addr += 0x1000;
  } else {
    vram_addr &= ~0x7000;
    int coarse_y = (vram_addr & 0x03E0) >> 5;
    if (coarse_y == 29) {
      coarse_y = 0;
      vram_addr ^= 0x0800;
    } else if (coarse_y == 31) {
      coarse_y = 0;
    } else {
      coarse_y++;
    }
    vram_addr = (vram_addr & ~0x03E0) | (coarse_y << 5);
  }

  // Reset x
  const int mask = 0x041F;
  vram_addr = (vram_addr & ~mask) | (temp_vram_addr & mask);
}

void PPU::render_scanline_sprites(uint8_t scanline_buffer[]) {
  int sprite_height = PPUCTRL.sprite_size ? 16 : 8;
  OAMEntry secondary_oam[8];
  memset(secondary_oam, 0xFF, 8 * sizeof(OAMEntry));

  // Evaluation
  int n = 0;
  bool sprite_0 = false;
  for (int i = 0; i < 64; i++) {
    uint8_t y = OAM[i * 4 + 0];
    if (y < 0xFF) {
      y++;  // sprites are shifted down 1 row
    }
    if (scanline >= y && scanline < y + sprite_height) {
      if (i == 0) {
        sprite_0 = true;
      }
      OAMEntry& entry = secondary_oam[n++];
      entry.y = y;
      entry.tile = OAM[i * 4 + 1];
      entry.attributes = OAM[i * 4 + 2];
      entry.x = OAM[i * 4 + 3];
      if (n == 8) {
        PPUSTATUS.sprite_overflow = 1;
        break;
      }
    }
  }

  // Render
  // TODO: 8x16
  for (int i = 0; i < n; i++) {
    OAMEntry& entry = secondary_oam[i];
    uint8_t priority = (entry.attributes >> 5) & 0x01;
    uint8_t flip_x = (entry.attributes >> 6) & 0x01;
    uint8_t flip_y = (entry.attributes >> 7) & 0x01;

    int start_x = entry.x;
    int y = (scanline - entry.y) % 8;
    if (flip_y) {
      y = 7 - y;
    }

    uint8_t tile_index = entry.tile;
    uint16_t pt_addr_base = (PPUCTRL.sprite_pt_addr << 12) | (tile_index << 4);
    uint8_t color_lo = mem_read(pt_addr_base | y);
    uint8_t color_hi = mem_read(pt_addr_base | 0x0008 | y);
    uint8_t palette_index = entry.attributes & 0x03;

    for (int x = 0; x < 8; x++) {
      int shift_x = flip_x ? x : 7 - x;
      uint8_t color_index = ((color_hi >> shift_x) & 0x1) << 1;
      color_index |= ((color_lo >> shift_x) & 0x1);

      uint8_t palette = color_index;
      if (color_index != 0) {
        palette |= (palette_index << 2);
        palette += 0x10;  // use sprite palettes
      }

      if (palette == 0) {
        continue;
      }

      if (scanline_buffer[start_x + x] == 0) {
        scanline_buffer[start_x + x] = palette;
      } else {
        if (i == 0 && sprite_0) {
          PPUSTATUS.sprite_0_hit = true;
        }
        if (priority == 0) {
          scanline_buffer[start_x + x] = palette;
        }
      }
    }
  }
}

void PPU::render_tile(int tile_index, int start_x, int start_y) {
  const uint8_t colors[4] = {0, 100, 200, 255};

  uint16_t addr_base = (tile_index << 4);
  for (int y = 0; y < 8; y++) {
    uint8_t color_lo = mem_read(addr_base | y);
    uint8_t color_hi = mem_read(addr_base | 0x0008 | y);

    for (int x = 0; x < 8; x++) {
      uint8_t color_index = ((color_hi >> (7 - x)) & 0x01) << 1;
      color_index |= ((color_lo >> (7 - x)) & 0x01);

      pixels[start_y + y][start_x + x][0] = colors[color_index];
      pixels[start_y + y][start_x + x][1] = colors[color_index];
      pixels[start_y + y][start_x + x][2] = colors[color_index];
    }
  }
}