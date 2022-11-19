#include "apu.h"
#include <cstdio>
#include <cstring>
#include "nes.h"

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
const uint16_t noise_timer_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};
const uint16_t dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};
const uint8_t length_table[32] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

// Mixer lookup tables
float pulse_table[31];
float tnd_table[203];

}  // namespace

APU::APU(NES& nes) : nes(nes), dmc(nes) {
  pulse[0].sweep_negate_tweak = 1;

  pulse_table[0] = 0.0f;
  for (int i = 1; i <= 30; i++) {
    pulse_table[i] = 95.52f / (8128.0f / i + 100);
  }
  tnd_table[0] = 0.0f;
  for (int i = 1; i <= 202; i++) {
    tnd_table[i] = 163.67f / (24329.0f / i + 100);
  }

  debug_waveforms[0] = WaveformCapture(15, 1);
  debug_waveforms[1] = WaveformCapture(15, 1);
  debug_waveforms[2] = WaveformCapture(15, 8);
  debug_waveforms[3] = WaveformCapture(15, 1);
  debug_waveforms[4] = WaveformCapture(127, 1);
}

void APU::power_on() {
  cycle = 0;
  memset(output_buffer, 0x00, sizeof(output_buffer));

  port_write(0x4017, 0x00);
  port_write(0x4015, 0x00);
  for (uint16_t addr = 0x4000; addr <= 0x4013; addr++) {
    port_write(addr, 0x00);
  }
  triangle.sequence_counter = 0;
  noise.shift_register = 0x0001;
  dmc.output_level &= 0x01;
}

uint8_t APU::port_read(uint16_t addr) {
  if (addr == 0x4015) {
    return read_status();
  }
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
  } else if (addr <= 0x400F) {
    noise.write_register(addr, value);
  } else if (addr <= 0x4013) {
    dmc.write_register(addr, value);
  } else if (addr == 0x4015) {
    write_status(value);
  } else if (addr == 0x4017) {
    write_frame_counter_control(value);
  }
}

void APU::write_status(uint8_t value) {
  pulse[0].enabled = (value >> 0) & 0x01;
  if (!pulse[0].enabled) {
    pulse[0].length_counter = 0;
  }
  pulse[1].enabled = (value >> 1) & 0x01;
  if (!pulse[1].enabled) {
    pulse[1].length_counter = 0;
  }
  triangle.enabled = (value >> 2) & 0x01;
  if (!triangle.enabled) {
    triangle.length_counter = 0;
  }
  noise.enabled = (value >> 3) & 0x01;
  if (!noise.enabled) {
    noise.length_counter = 0;
  }
  dmc.set_enabled((value >> 4) & 0x01);
}

uint8_t APU::read_status() {
  uint8_t value = 0;
  value |= (pulse[0].length_counter > 0);
  value |= (pulse[1].length_counter > 0) << 1;
  value |= (triangle.length_counter > 0) << 2;
  value |= (noise.length_counter > 0) << 3;
  value |= (dmc.bytes_left > 0) << 4;
  value |= frame_interrupt_flag << 6;
  value |= dmc.interrupt_flag << 7;
  frame_interrupt_flag = false;
  // Note: DMC interrupt flag is -not- cleared
  return value;
}

void APU::write_frame_counter_control(uint8_t value) {
  frame_counter_mode = value >> 7;
  frame_counter_irq_inhibit = (value >> 6) & 0x01;
  if (frame_counter_irq_inhibit) {
    frame_interrupt_flag = false;
  }

  if (frame_counter_mode == 1) {
    // Immediately clock all units
    frame_counter_step = 3;
    cycle = frame_counter_cycles[frame_counter_mode][frame_counter_step] - 1;
    update_frame_counter();
  } else {
    cycle = 0;
    frame_counter_step = 0;
  }
}

void APU::update_frame_counter() {
  if (cycle++ == frame_counter_cycles[frame_counter_mode][frame_counter_step]) {
    for (int i = 0; i < 2; i++) {
      pulse[i].envelope.update();
    }
    triangle.update_linear_counter();
    noise.envelope.update();

    if (frame_counter_step % 2 == 1) {
      for (int i = 0; i < 2; i++) {
        pulse[i].length_counter.update();
        pulse[i].update_sweep();
      }
      triangle.length_counter.update();
      noise.length_counter.update();
    }

    frame_counter_step++;
    if (frame_counter_step >= 4) {
      frame_counter_step = 0;
      cycle = 0;
      if (frame_counter_mode == 0 && !frame_counter_irq_inhibit) {
        frame_interrupt_flag = true;
      }
    }
  }
}

void APU::tick() {
  update_frame_counter();

  // Update timers
  if (cycle % 2 == 0) {
    for (int i = 0; i < 2; i++) {
      pulse[i].update_timer();
    }
    noise.update_timer();
  }
  triangle.update_timer();
  dmc.update_timer();

  // Sample and mix output
  sample_cycle++;
  if (sample_cycle >= cycles_per_sample) {
    sample_cycle -= cycles_per_sample;
    sample();
  }

  // Set IRQ
  if (frame_interrupt_flag || dmc.interrupt_flag) {
    nes.cpu.set_IRQ();
  }
}

void APU::sample() {
  uint8_t pulse_index = 0;
  for (int i = 0; i < 2; i++) {
    uint8_t pulse_out = pulse[i].output();
    pulse_index += pulse_out;
    debug_waveforms[i].add_sample(pulse_out);
  }
  uint8_t tnd_index = 0;

  uint8_t triangle_out = triangle.output();
  tnd_index += triangle_out * 3;
  debug_waveforms[2].add_sample(triangle_out);

  uint8_t noise_out = noise.output();
  tnd_index += noise_out * 2;
  debug_waveforms[3].add_sample(noise_out);

  uint8_t dmc_out = dmc.output();
  tnd_index += dmc_out;
  debug_waveforms[4].add_sample(dmc_out);

  float pulse_out = pulse_table[pulse_index];
  float tnd_out = tnd_table[tnd_index];
  output_buffer[sample_count] = (pulse_out + tnd_out) * INT16_MAX;
  sample_count++;
}

void APU::output() {
  sample_count = 0;
}

void LengthCounter::load(uint8_t index) {
  if (enabled) {
    counter = length_table[index];
  }
}

void LengthCounter::update() {
  if (counter > 0 && !halt) {
    counter--;
  }
}

void Envelope::load(uint8_t data) {
  constant_volume = (data >> 4) & 0x01;
  volume_or_period = data & 0x0F;
}

void Envelope::update() {
  if (start) {
    start = false;
    decay_level_counter = 15;
    divider = volume_or_period;
  } else {
    if (divider == 0) {
      divider = volume_or_period;
      if (decay_level_counter == 0) {
        if (loop) {
          decay_level_counter = 15;
        }
      } else {
        decay_level_counter--;
      }
    } else {
      divider--;
    }
  }
}

uint8_t Envelope::volume() {
  return constant_volume ? volume_or_period : decay_level_counter;
}

void Pulse::write_register(uint16_t addr, uint8_t value) {
  switch (addr & 0xFFF3) {
    case 0x4000:
      duty = (value >> 6) & 0x03;
      fc_halt_or_loop = (value >> 5) & 0x01;
      envelope.load(value);
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
      length_counter.load(value >> 3);
      sequence_counter = 0;
      envelope.start = true;
      break;
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
  if (!enabled || sweep_muted() || length_counter == 0) {
    return 0;
  }
  return pulse_sequence[duty][sequence_counter] * envelope.volume();
}

void Triangle::write_register(uint16_t addr, uint8_t value) {
  switch (addr) {
    case 0x4008:
      fc_halt_or_linear_control = (value >> 7) & 0x01;
      linear_counter_period = value & 0x7F;
      break;
    case 0x400A:
      timer_period = (timer_period & 0xFF00) | value;
      break;
    case 0x400B:
      timer_period = ((value & 0x07) << 8) | (timer_period & 0x00FF);
      length_counter.load(value >> 3);
      linear_counter_reload = true;
      break;
  }
}

void Triangle::update_linear_counter() {
  if (linear_counter_reload) {
    linear_counter = linear_counter_period;
  } else if (linear_counter > 0) {
    linear_counter--;
  }
  if (!fc_halt_or_linear_control) {
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
  if (!enabled) {
    return 0;
  }
  return triangle_sequence[sequence_counter];
}

void Noise::write_register(uint16_t addr, uint8_t value) {
  switch (addr) {
    case 0x400C:
      fc_halt_or_loop = (value >> 5) & 0x01;
      envelope.load(value);
      break;
    case 0x400E:
      mode = value >> 7;
      timer_period = noise_timer_period_table[value & 0x0F];
      break;
    case 0x400F:
      length_counter.load(value >> 3);
      envelope.start = true;
      break;
  }
}

void Noise::update_timer() {
  if (timer-- == 0) {
    timer = timer_period;
    int bit = mode ? 6 : 1;
    uint8_t feedback = (shift_register ^ (shift_register >> bit)) & 0x0001;
    shift_register = (feedback << 14) | (shift_register >> 1);
  }
}

uint8_t Noise::output() {
  if (!enabled || (shift_register & 0x0001) || length_counter == 0) {
    return 0;
  }
  return envelope.volume();
}

void DMC::write_register(uint16_t addr, uint8_t value) {
  switch (addr) {
    case 0x4010:
      irq_enabled = value >> 7;
      loop = (value >> 6) & 0x01;
      rate = dmc_rate_table[value & 0x0F];
      if (!irq_enabled) {
        interrupt_flag = false;
      }
      break;
    case 0x4011:
      output_level = value & 0x7F;
      break;
    case 0x4012:
      sample_address = 0xC000 + value * 64;
      break;
    case 0x4013:
      sample_length = value * 16 + 1;
      break;
  }
}

void DMC::set_enabled(bool value) {
  interrupt_flag = false;
  enabled = value;
  if (enabled) {
    if (bytes_left == 0) {
      restart_sample();
    }
  } else {
    bytes_left = 0;
  }
}

void DMC::restart_sample() {
  current_address = sample_address;
  bytes_left = sample_length;
}

void DMC::update_timer() {
  // Memory reader
  if (!sample_buffer_filled && bytes_left > 0) {
    // TODO: Emulate cpu stall
    sample_buffer = nes.cpu.mem_read(current_address, false);
    sample_buffer_filled = true;
    if (++current_address == 0) {
      current_address = 0x8000;
    }
    if (--bytes_left == 0) {
      if (loop) {
        restart_sample();
      } else if (irq_enabled) {
        interrupt_flag = true;
      }
    }
  }

  // Output unit
  if (timer-- == 0) {
    timer = rate - 1;
    if (bits_left == 0) {
      // New cycle
      bits_left = 8;
      silenced = !sample_buffer_filled;
      if (sample_buffer_filled) {
        // Empty sample buffer into shift register
        shift_register = sample_buffer;
        sample_buffer_filled = false;
      }
    }
    if (!silenced) {
      int change = (shift_register & 0x01) ? 2 : -2;
      int new_level = output_level + change;
      if (new_level >= 0 && new_level <= 127) {
        output_level = new_level;
      }
    }
    shift_register >>= 1;
    bits_left--;
  }
}

uint8_t DMC::output() {
  if (!enabled) {
    return 0;
  }
  return output_level;
}