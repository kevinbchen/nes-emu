#pragma once
#include <cstdint>
#include "bitfield.h"

class NES;

struct Pulse {
  // Properties
  uint8_t duty = 0;
  bool length_counter_halt = false;
  bool constant_volume = true;
  uint8_t volume_or_period = 0;
  bool sweep = false;
  uint8_t sweep_period = 0;
  bool sweep_negate = false;
  uint8_t sweep_negate_tweak = 0;
  uint8_t sweep_shift = 0;
  uint8_t length_counter = 0;
  uint16_t timer_period = 0;

  // Counters / flags
  uint16_t timer = 0;
  uint8_t sequence_counter = 0;
  uint16_t target_period = 0;
  bool envelope_start = false;
  uint8_t decay_level_counter = 0;
  uint8_t envelope_divider = 0;
  uint8_t sweep_divider = 0;
  bool sweep_reload = false;

  void calculate_target_period();

  void write_register(uint16_t addr, uint8_t value);
  void update_envelope();
  void update_length_counter();
  bool sweep_muted();
  void update_sweep();
  void update_timer();
  uint8_t output();
};

class APU {
 public:
  int16_t output_buffer[735 + 1000];
  int sample_count = 0;

  APU(NES& nes);
  void power_on();
  void output();

  uint8_t port_read(uint16_t addr);
  void port_write(uint16_t addr, uint8_t value);

  void tick();

 private:
  NES& nes;

  int cycle = 0;
  int step = 0;

  float sample_cycle = 0;

  void sample();

  // Channels
  Pulse pulse[2];

  // Registers
  void write_pulse_register(uint16_t addr, uint8_t value);
};
