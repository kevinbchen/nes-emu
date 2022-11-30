#pragma once
#include <cstdint>
#include "waveform_capture.h"

class NES;

struct LengthCounter {
  bool& enabled;
  bool& halt;
  uint8_t counter = 0;

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
  bool constant_volume = false;
  uint8_t volume_or_period = 0;
  bool start = false;
  uint8_t decay_level_counter = 0;
  uint8_t divider = 0;

  Envelope(bool& loop) : loop(loop) {}
  void load(uint8_t data);
  void update();
  uint8_t volume();
};

struct Pulse {
  // Properties
  bool enabled = false;
  uint8_t duty = 0;
  bool fc_halt_or_loop = false;
  bool sweep = false;
  uint8_t sweep_period = 0;
  bool sweep_negate = false;
  uint8_t sweep_negate_tweak = 0;
  uint8_t sweep_shift = 0;
  uint16_t timer_period = 0;

  // Counters / flags
  uint16_t timer = 0;
  uint8_t sequence_counter = 0;
  uint16_t target_period = 0;
  uint8_t sweep_divider = 0;
  bool sweep_reload = false;
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
  bool enabled = false;
  bool fc_halt_or_linear_control = false;
  uint8_t linear_counter_period = 0;
  uint16_t timer_period = 0;

  // Counters / flags
  uint16_t timer = 0;
  uint8_t sequence_counter = 0;
  uint8_t linear_counter = 0;
  bool linear_counter_reload = false;
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
  bool enabled = false;
  bool fc_halt_or_loop = false;
  bool mode = false;
  uint8_t timer_period = 0;

  // Counters / flags
  uint16_t timer = 0;
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
  bool enabled = false;
  bool loop = false;
  bool irq_enabled = false;
  uint16_t rate = 0;
  uint16_t sample_address = 0;
  uint16_t sample_length = 0;

  // Counters / flags
  uint16_t timer = 0;
  uint16_t bytes_left = 0;
  uint16_t current_address = 0;
  uint8_t sample_buffer = 0;
  bool sample_buffer_filled = false;
  bool interrupt_flag = false;

  uint8_t shift_register = 0;
  uint8_t bits_left = 0;
  uint8_t output_level = 0;
  bool silenced = false;

  DMC(NES& nes) : nes(nes) {}
  void write_register(uint16_t addr, uint8_t value);
  void set_enabled(bool value);
  void restart_sample();
  void update_timer();
  uint8_t output();
};

class APU {
 public:
  static constexpr int max_output_buffer_size = 1000;
  int16_t output_buffer[max_output_buffer_size];
  int sample_count = 0;
  WaveformCapture debug_waveforms[5];

  APU(NES& nes);
  void power_on();
  void clear_output_buffer();
  void set_sample_rate(int rate);

  uint8_t port_read(uint16_t addr);
  void port_write(uint16_t addr, uint8_t value);

  void tick();

 private:
  NES& nes;

  int cycle = 0;
  float sample_cycle = 0;
  int sample_rate;
  float cycles_per_sample;

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
  bool frame_counter_irq_inhibit = false;
  bool frame_interrupt_flag = false;

  void write_frame_counter_control(uint8_t value);
  void update_frame_counter();
};
