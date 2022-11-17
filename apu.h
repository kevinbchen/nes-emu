#pragma once
#include <cstdint>
#include "bitfield.h"

class NES;

struct Pulse {
  // Properties
  uint8_t duty;
  bool length_counter_halt;
  bool constant_volume;
  uint8_t volume_or_period;
  bool sweep;
  uint8_t sweep_period;
  bool sweep_negate;
  uint8_t sweep_negate_tweak;
  uint8_t sweep_shift;
  uint8_t length_counter;
  uint16_t timer_period;

  // Counters / flags
  uint16_t timer;
  uint8_t sequence_counter;
  uint16_t target_period;
  bool envelope_start;
  uint8_t decay_level_counter;
  uint8_t envelope_divider;
  uint8_t sweep_divider;
  bool sweep_reload;

  void calculate_target_period();

  void write_register(uint16_t addr, uint8_t value);
  void update_envelope();
  void update_length_counter();
  bool sweep_muted();
  void update_sweep();
  void update_timer();
  uint8_t output();
};

struct Triangle {
  // Properties
  bool length_counter_halt;
  uint8_t linear_counter_period;
  uint16_t timer_period;
  uint8_t length_counter;

  // Counters / flags
  uint16_t timer;
  uint8_t sequence_counter;
  uint8_t linear_counter;
  bool linear_counter_reload;

  void write_register(uint16_t addr, uint8_t value);
  void update_length_counter();
  void update_linear_counter();
  void update_timer();
  uint8_t output();
};

struct Noise {
  // Properties
  bool length_counter_halt;
  bool constant_volume;
  uint8_t volume_or_period;
  bool mode;
  uint8_t timer_period;
  uint8_t length_counter;

  // Counters / flags
  uint16_t timer;
  bool envelope_start;
  uint8_t decay_level_counter;
  uint8_t envelope_divider;
  uint16_t shift_register = 0x0001;

  void write_register(uint16_t addr, uint8_t value);
  void update_envelope();
  void update_length_counter();
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
  float sample_cycle = 0;

  void sample();

  // Channels
  Pulse pulse[2];
  Triangle triangle;
  Noise noise;

  union {
    uint8_t raw;
    BitField8<0, 1> enable_pulse1;
    BitField8<1, 1> enable_pulse2;
    BitField8<2, 1> enable_triangle;
    BitField8<3, 1> enable_noise;
    BitField8<4, 1> enable_dmc;
  } status;
  void write_status(uint8_t value);
  uint8_t read_status();

  int frame_counter_step = 0;
  union {
    uint8_t raw;
    BitField8<6, 1> mode;
    BitField8<7, 1> irq_inhibit;
  } frame_counter_control;
  void write_frame_counter_control(uint8_t value);
};
