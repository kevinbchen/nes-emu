#pragma once
#include <cstdint>

class WaveformCapture {
 public:
  static constexpr int buffer_size = 735;
  float output_buffer[buffer_size];

  WaveformCapture() {}
  WaveformCapture(uint8_t max_value, uint8_t trigger_level);
  void add_sample(uint8_t value);

 private:
  uint8_t buffer[buffer_size];
  int buffer_index = 0;

  uint8_t max_value = 255;
  uint8_t trigger_level = 0;
  uint8_t last_value = 0;
  bool capturing = false;
  int capture_count = 0;
  int non_capture_count = 0;

  void output();
};