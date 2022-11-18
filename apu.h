#pragma once
#include <cstdint>
#include <cstdio>

class NES;

struct LengthCounter {
  bool& enabled;
  bool& halt;
  uint8_t counter;

  LengthCounter(bool& enabled, bool& halt) : enabled(enabled), halt(halt) {}
  operator uint8_t() const { return counter; }
  LengthCounter& operator=(uint8_t v) {
    counter = v;
    return *this;
  }
  void load(uint8_t index);
  void update();
};

struct Envelope {
  bool& loop;
  bool constant_volume;
  uint8_t volume_or_period;
  bool start;
  uint8_t decay_level_counter;
  uint8_t divider;

  Envelope(bool& loop) : loop(loop) {}
  void load(uint8_t data);
  void update();
  uint8_t volume();
};

struct Pulse {
  // Properties
  bool enabled;
  uint8_t duty;
  bool fc_halt_or_loop;
  bool sweep;
  uint8_t sweep_period;
  bool sweep_negate;
  uint8_t sweep_negate_tweak;
  uint8_t sweep_shift;
  uint16_t timer_period;

  // Counters / flags
  uint16_t timer;
  uint8_t sequence_counter;
  uint16_t target_period;
  uint8_t sweep_divider;
  bool sweep_reload;
  LengthCounter length_counter;
  Envelope envelope;

  Pulse()
      : length_counter(enabled, fc_halt_or_loop), envelope(fc_halt_or_loop) {}
  void calculate_target_period();
  void write_register(uint16_t addr, uint8_t value);
  bool sweep_muted();
  void update_sweep();
  void update_timer();
  uint8_t output();
};

struct Triangle {
  // Properties
  bool enabled;
  bool fc_halt_or_linear_control;
  uint8_t linear_counter_period;
  uint16_t timer_period;

  // Counters / flags
  uint16_t timer;
  uint8_t sequence_counter;
  uint8_t linear_counter;
  bool linear_counter_reload;
  LengthCounter length_counter;

  Triangle() : length_counter(enabled, fc_halt_or_linear_control) {}
  void write_register(uint16_t addr, uint8_t value);
  void update_length_counter();
  void update_linear_counter();
  void update_timer();
  uint8_t output();
};

struct Noise {
  // Properties
  bool enabled;
  bool fc_halt_or_loop;
  bool mode;
  uint8_t timer_period;

  // Counters / flags
  uint16_t timer;
  uint16_t shift_register = 0x0001;
  LengthCounter length_counter;
  Envelope envelope;

  Noise()
      : length_counter(enabled, fc_halt_or_loop), envelope(fc_halt_or_loop) {}
  void write_register(uint16_t addr, uint8_t value);
  void update_length_counter();
  void update_timer();
  uint8_t output();
};

struct DMC {
  // Properties
  NES& nes;
  bool enabled;
  bool loop;
  bool irq_enabled;
  uint16_t rate;
  uint16_t sample_address;
  uint16_t sample_length;

  // Counters / flags
  uint16_t timer;
  uint16_t bytes_left;
  uint16_t current_address;
  uint8_t sample_buffer;
  bool sample_buffer_filled;
  bool interrupt_flag;

  uint8_t shift_register;
  uint8_t bits_left;
  uint8_t output_level;
  bool silenced;

  DMC(NES& nes) : nes(nes) {}
  void write_register(uint16_t addr, uint8_t value);
  void set_enabled(bool value);
  void restart_sample();
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
  DMC dmc;

  void write_status(uint8_t value);
  uint8_t read_status();

  uint8_t frame_counter_step = 0;
  uint8_t frame_counter_mode = 0;
  bool frame_counter_irq_inhibit;
  bool frame_interrupt_flag;

  void write_frame_counter_control(uint8_t value);
  void update_frame_counter();
};
