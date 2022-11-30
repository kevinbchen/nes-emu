#include "waveform_capture.h"

WaveformCapture::WaveformCapture(uint8_t max_value, uint8_t trigger_level)
    : max_value(max_value), trigger_level(trigger_level), output_buffer() {}

void WaveformCapture::add_sample(uint8_t value) {
  buffer[buffer_index] = value;

  if (capturing) {
    capture_count++;
    if (capture_count == buffer_size) {
      // Output buffer around trigger
      output();
      capturing = false;
    }
  } else {
    non_capture_count++;
    if (last_value < trigger_level && value >= trigger_level) {
      capturing = true;
      capture_count = buffer_size / 2;
      non_capture_count = 0;
    } else if (non_capture_count == buffer_size) {
      // Didn't trigger for an entire buffer
      output();
      non_capture_count = 0;
    }
  }

  last_value = value;
  buffer_index = (buffer_index + 1) % buffer_size;
}

void WaveformCapture::output() {
  int j = (buffer_index + 1) % buffer_size;
  for (int i = 0; i < buffer_size; i++) {
    output_buffer[i] = (float)buffer[j] / max_value;
    j = (j + 1) % buffer_size;
  }
}