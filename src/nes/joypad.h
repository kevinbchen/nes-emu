#pragma once
#include <cstdint>

enum class Button : int { A = 0, B, Select, Start, Up, Down, Left, Right };

class Joypad {
 public:
  Joypad();

  uint8_t port_read(uint16_t addr);
  void port_write(uint16_t addr, uint8_t value);

  void set_button_state(int joypad, Button button, bool pressed);

 private:
  bool strobe = false;
  uint8_t shift_register[2];
  bool button_state[2][8];

  void set_shift_registers();
};
