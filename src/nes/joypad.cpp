#include "joypad.h"
#include <cstdio>

Joypad::Joypad() {
  for (int i = 0; i < 2; i++) {
    shift_register[i] = 0;
    for (int j = 0; j < 8; j++) {
      button_state[i][j] = 0;
    }
  }
}

uint8_t Joypad::port_read(uint16_t addr) {
  if (!(addr == 0x4016 || addr == 0x4017)) {
    return 0;
  }
  if (strobe) {
    set_shift_registers();
  }
  int i = addr & 0x01;
  uint8_t value = shift_register[i] & 0x01;
  shift_register[i] = (shift_register[i] >> 1) | 0x80;
  return value | 0x40;
}

void Joypad::port_write(uint16_t addr, uint8_t value) {
  if (addr == 0x4016) {
    bool new_strobe = value & 0x01;
    if (strobe && !new_strobe) {
      set_shift_registers();
    }
    strobe = new_strobe;
  }
}

void Joypad::set_button_state(int joypad, Button button, bool pressed) {
  button_state[joypad][(int)button] = pressed;
}

void Joypad::set_shift_registers() {
  for (int i = 0; i < 2; i++) {
    shift_register[i] = 0;
    for (int j = 0; j < 8; j++) {
      shift_register[i] |= (button_state[i][j] << j);
    }
  }
}