#include "apu.h"
#include <cstdio>
#include <cstring>

namespace {

constexpr int sample_rate = 44100;
constexpr float cycles_per_sample = 1789773.0f / sample_rate;

const int frame_counter_cycles[2][4] = {
    {7457, 14913, 22371, 29829},
    {7457, 14913, 22371, 37281},
};
const uint8_t pulse_sequence[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0},
};
const uint8_t triangle_sequence[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};
const uint8_t length_table[32] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

// Mixer lookup tables
float pulse_table[31];
float tnd_table[203];

}  // namespace

APU::APU(NES& nes) : nes(nes) {
  pulse[0].sweep_negate_tweak = 1;

  pulse_table[0] = 0.0f;
  for (int i = 1; i <= 30; i++) {
    pulse_table[i] = 95.52f / (8128.0f / i + 100);
  }
  tnd_table[0] = 0.0f;
  for (int i = 1; i <= 202; i++) {
    tnd_table[i] = 163.67f / (24329.0f / i + 100);
  }
}

void APU::power_on() {
  cycle = 0;
  memset(output_buffer, 0x00, sizeof(output_buffer));
}

uint8_t APU::port_read(uint16_t addr) {
  return 0;
}

void APU::port_write(uint16_t addr, uint8_t value) {
  if (addr < 0x4000) {
    return;
  } else if (addr <= 0x4007) {
    int index = addr < 0x4004 ? 0 : 1;
    pulse[index].write_register(addr, value);
  } else if (addr <= 0x400B) {
    triangle.write_register(addr, value);
  } else if (addr == 0x4015) {
    write_status(value);
  }
}

void APU::write_status(uint8_t value) {
  status.raw = value;
  if (!status.enable_pulse1) {
    pulse[0].length_counter = 0;
  }
  if (!status.enable_pulse2) {
    pulse[1].length_counter = 0;
  }
  if (!status.enable_triangle) {
    triangle.length_counter = 0;
  }
  // TODO: clear DMC interrupt flag
}

void APU::tick() {
  // Update frame counter
  if (cycle++ == frame_counter_cycles[0][step]) {
    for (int i = 0; i < 2; i++) {
      pulse[i].update_envelope();
    }
    triangle.update_linear_counter();

    if (step & 0x1) {
      for (int i = 0; i < 2; i++) {
        pulse[i].update_length_counter();
        pulse[i].update_sweep();
      }
      triangle.update_length_counter();
    }
    step++;
    if (step >= 4) {
      step = 0;
      cycle = 0;
    }
  }

  // Update timers
  if (cycle % 2 == 0) {
    for (int i = 0; i < 2; i++) {
      pulse[i].update_timer();
    }
  }
  triangle.update_timer();

  // Sample and mix output
  sample_cycle++;
  if (sample_cycle >= cycles_per_sample) {
    sample_cycle -= cycles_per_sample;
    sample();
  }
}

void APU::sample() {
  uint8_t pulse_index = 0;
  if (status.enable_pulse1) {
    pulse_index += pulse[0].output();
  }
  if (status.enable_pulse2) {
    pulse_index += pulse[1].output();
  }
  uint8_t tnd_index = 0;
  if (status.enable_triangle) {
    tnd_index += triangle.output() * 3;
  }

  float pulse_out = pulse_table[pulse_index];
  float tnd_out = tnd_table[tnd_index];

  output_buffer[sample_count] = (pulse_out + tnd_out) * INT16_MAX;
  sample_count++;
}

void APU::output() {
  sample_count = 0;
}

void Pulse::write_register(uint16_t addr, uint8_t value) {
  switch (addr & 0xFFF3) {
    case 0x4000:
      duty = (value >> 6) & 0x03;
      length_counter_halt = (value >> 5) & 0x01;
      constant_volume = (value >> 4) & 0x01;
      volume_or_period = value & 0x0F;
      break;
    case 0x4001:
      sweep = value >> 7;
      sweep_period = (value & 0x70) >> 4;
      sweep_negate = (value >> 3) & 0x01;
      sweep_shift = value & 0x07;
      calculate_target_period();
      sweep_reload = true;
      break;
    case 0x4002:
      timer_period = (timer_period & 0xFF00) | value;
      calculate_target_period();
      break;
    case 0x4003:
      timer_period = ((value & 0x07) << 8) | (timer_period & 0x00FF);
      calculate_target_period();
      length_counter = length_table[value >> 3];
      sequence_counter = 0;
      envelope_start = true;
      break;
  }
}

void Pulse::update_envelope() {
  if (envelope_start) {
    envelope_start = false;
    decay_level_counter = 15;
    envelope_divider = volume_or_period;
  } else {
    if (envelope_divider == 0) {
      envelope_divider = volume_or_period;
      if (decay_level_counter == 0) {
        if (length_counter_halt) {
          decay_level_counter = 15;
        }
      } else {
        decay_level_counter--;
      }
    } else {
      envelope_divider--;
    }
  }
}

void Pulse::update_length_counter() {
  if (length_counter > 0 && !length_counter_halt) {
    length_counter--;
  }
}

void Pulse::calculate_target_period() {
  int change = timer_period >> sweep_shift;
  if (sweep_negate) {
    change = -(change + sweep_negate_tweak);
  }
  target_period = (int)timer_period + change;
}

bool Pulse::sweep_muted() {
  return timer_period < 8 || target_period > 0x07FF;
}

void Pulse::update_sweep() {
  if (sweep && sweep_divider == 0 && !sweep_muted()) {
    timer_period = target_period;
    calculate_target_period();
  }
  if (sweep_divider == 0 || sweep_reload) {
    sweep_divider = sweep_period;
    sweep_reload = false;
  } else {
    sweep_divider--;
  }
}

void Pulse::update_timer() {
  if (timer-- == 0) {
    timer = timer_period;
    sequence_counter = (sequence_counter - 1 + 8) % 8;
  }
}

uint8_t Pulse::output() {
  if (sweep_muted() || length_counter == 0) {
    return 0;
  }
  uint8_t volume = constant_volume ? volume_or_period : decay_level_counter;
  return pulse_sequence[duty][sequence_counter] * volume;
}

void Triangle::write_register(uint16_t addr, uint8_t value) {
  switch (addr) {
    case 0x4008:
      length_counter_halt = (value >> 7) & 0x01;
      linear_counter_period = value & 0x7F;
      break;
    case 0x400A:
      timer_period = (timer_period & 0xFF00) | value;
      break;
    case 0x400B:
      timer_period = ((value & 0x07) << 8) | (timer_period & 0x00FF);
      length_counter = length_table[value >> 3];
      linear_counter_reload = true;
      break;
  }
}

void Triangle::update_length_counter() {
  if (length_counter > 0 && !length_counter_halt) {
    length_counter--;
  }
}

void Triangle::update_linear_counter() {
  if (linear_counter_reload) {
    linear_counter = linear_counter_period;
  } else if (linear_counter > 0) {
    linear_counter--;
  }
  if (!length_counter_halt) {
    linear_counter_reload = false;
  }
}

void Triangle::update_timer() {
  if (timer-- == 0) {
    timer = timer_period;
    if (linear_counter > 0 && length_counter > 0 && timer_period > 3) {
      sequence_counter = (sequence_counter - 1 + 32) % 32;
    }
  }
}

uint8_t Triangle::output() {
  return triangle_sequence[sequence_counter];
}
